#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>


#define PORT 8080 /* port of server and client */

#define DATABASE_PATH "../database/super-secret.db"

#define BUF_PRS_SIZE 72
#define BUF_ANS_SIZE 32

#define LAST_QUESTION 'l'
#define NEXT_QUESTION 'n'
#define QUESTION 'q'
#define PROCEED 'p'
#define DISCARD 'd'
#define ANSWER 'a'
#define CLUE 'c'
#define EXIT 'e'

#define TO_INT(c) ((c) - '0')

int quit = 0;


/// @brief structure to hold the question, answer, and clue
typedef struct {
    char *question, *answer, *clue;
    int question_length, answer_length, clue_length;
} Question;

/// @brief structure to hold the question node
typedef struct QuestionNode {
    Question *question;
    struct QuestionNode *next;
} QuestionNode;


void sigint_handler() { quit = 1; }

void sigpipe_handler() { signal(SIGPIPE, sigpipe_handler); }




/// @brief frees all the memory allocated
/// @param head head of linked list
void clear(QuestionNode **head) {
    QuestionNode *node;

    while (*head != NULL) {
        node = *head;
        *head = (**head).next;
        free((*(*node).question).question);
        free((*(*node).question).answer);
        free((*(*node).question).clue);
        free((*node).question);
        free(node);
    }
}


/// @brief duplicates a string
/// @param src string to be duplicated
/// @return pointer to a null-terminated string, which is a duplicate of the src string
char *duplicate(char *src) {
    char *str, *p;
    int len = 0;

    while (src[len]) { len++; }
    str = malloc(len + 1);
    p = str;
    while (*src) { *(p++) = *(src++); }
    *p = '\0';

    return str;
}


/// @brief parses the next line from the database
/// @param fd file descriptor of database
/// @param buf buffer to store the information of the line
/// @param len length of line
/// @return 0 and line length if parsed successfully, 1 otherwise
int parse_line(int fd, char *buf, int *len) {
    char c;
    int n, count = 0, bytes = 0;

    while ((n = read(fd, &c, 1)) == 1 && c >= '0' && c <= '9') { 
        bytes = (bytes * 10) + TO_INT(c);
        count++;
    }
    if (n != 1) { /* EOF */
        close(fd);
        return 1;
    }
    if (!count) {
        printf("database is not formatted: line needs to start with its size\n");
        close(fd);
        return 1;
    }
    buf[0] = c;
    read(fd, buf + 1, bytes); /* newline included */
    buf[bytes] = '\0';

    *len = bytes;

    return 0;
}


/// @brief parses the database into a linked list
/// @param head head of linked list to store the questions
/// @return 0 if parsed successfully, 1 otherwise
int parser(QuestionNode **head) {
    int fd, len;
    char buf[BUF_PRS_SIZE];

    if ((fd = open(DATABASE_PATH, O_RDONLY)) == -1) {
        printf("failed to open database\n");
        return 1;
    }
    while (1) {  /* we will parse 3 lines in each iteration, until we reach EOF */
        Question *q = malloc(sizeof(Question));
        QuestionNode *node = malloc(sizeof(QuestionNode));

        if (q == NULL || node == NULL) {
            printf("memory error\n");
            clear(head);
            close(fd);
            return 1;
        }
        
        /* parse line (Question) - will return 1 if EOF is reached */
        if (parse_line(fd, buf, &len)) {
            free(q);
            free(node);
            break;
        }
        (*q).question = duplicate(buf);
        (*q).question_length = len;
        
        /* parse line (Answer) */
        parse_line(fd, buf, &len);
        (*q).answer = duplicate(buf);
        (*q).answer_length = len;

        /* parse line (Clue) */
        parse_line(fd, buf, &len);
        (*q).clue = duplicate(buf);
        (*q).clue_length = len;

        (*node).question = q;

        /* insert node in the beginning of linked list */
        (*node).next = *head;
        *head = node;
    }
    return 0;
}


/// @brief handles the client
/// @param client_socket_fd descriptor of the client socket
/// @param head head of linked list
void handle_client(int client_socket_fd, QuestionNode **head) {
    int n, i = 0;
    QuestionNode *node = *head;

    printf("client log:\n");

    while (node != NULL) {
        char c;
        int next_question = 0;

        // 1. the server starts by writing the status of the question to the client, i.e,
        //      if we not in the last question, the server sends PROCEED
        //      if we are in the last question, the server sends LAST_QUESTION
        //      if the server has to terminate the connection, the server sends DISCARD
        if ((*node).next == NULL) { c = LAST_QUESTION; }
        else if (quit) { c = DISCARD; }
        else { c = PROCEED; }

        send(client_socket_fd, &c, 1, 0);

        while ((n = read(client_socket_fd, &c, 1)) == 1) {
            switch (c) {
                case QUESTION:
                    send(client_socket_fd, (*(*node).question).question, (*(*node).question).question_length + 1, 0);
                    printf("question %d\n", i++);
                    break;
                case ANSWER:
                    send(client_socket_fd, (*(*node).question).answer, (*(*node).question).answer_length + 1, 0);
                    printf("answered\n");
                    break;
                case CLUE:
                    send(client_socket_fd, (*(*node).question).clue, (*(*node).question).clue_length + 1, 0);
                    printf("clue\n");
                    break;
                case NEXT_QUESTION:
                    next_question = 1;
                    break;
                case EXIT:
                    printf("client disconnected: /exit command\n");
                    return;
            }
            if (next_question) { break; }
        }
        if (n == 0) {
            printf("client disconnected\n");
            return;
        }
        node = (*node).next;
    }
}




int main() {
    int server_socket_fd, client_socket_fd;
    struct sockaddr_in address; // struct that holds the address of the server
    int socket_options = 1; /* to enable the sockets options */
    int addrlen = sizeof(address);
    QuestionNode *linked_list_questions_head = NULL;

    if (parser(&linked_list_questions_head)) {
        printf("failed to parse the database\n");
        return 1;
    }
    printf("database parsed successfully\n");

    // creates the server socket of type SOCK_STREAM, domain AF_INET (IPv4), protocol 0 (default)
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        clear(&linked_list_questions_head);
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    // set socket options to reuse the address and port number immediately after the server terminates
    // this is useful when the server crashes and you want to restart it without waiting for the port to be released
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &socket_options, sizeof(socket_options))) {
        clear(&linked_list_questions_head);
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // address family is AF_INET, IP address is INADDR_ANY, and port number is PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY means the server will bind to all network interfaces on the host machine
    address.sin_port = htons(PORT);

    // bind the server socket to the address and port number
    if (bind(server_socket_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        clear(&linked_list_questions_head);
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    if (listen(server_socket_fd, 3) < 0) {
        clear(&linked_list_questions_head);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept incoming connection from client and create a new socket for the client
    if ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        clear(&linked_list_questions_head);
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("connection established with client.\n");

    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, sigpipe_handler);

    handle_client(client_socket_fd, &linked_list_questions_head);

    if (quit) { printf("server terminated successfully by SIGINT\n"); }
    else { printf("client disconnected\n"); }

    clear(&linked_list_questions_head);

    close(server_socket_fd);
    close(client_socket_fd);

    return 0;
}
