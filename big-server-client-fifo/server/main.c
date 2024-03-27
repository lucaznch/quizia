#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#define DATABASE_PATH "../database/super-secret.db"

#define MAX_CLIENTS 2

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


typedef struct {
    char *question, *answer, *clue;
    int question_length, answer_length, clue_length;
} Question;

typedef struct QuestionNode {
    Question *question;
    struct QuestionNode *next;
} QuestionNode;

typedef struct {
    int id;
    char *request_fifo_path, *response_fifo_path;
} Client;

typedef struct {
    int *producer_ptr, *consumer_ptr, *count, *status;
    pthread_mutex_t *mtx;
    pthread_cond_t *producer_cond;
    pthread_cond_t *consumer_cond;
    Client **client;
    QuestionNode **head;
} ServerClient; 





void sigint_handler() { quit = 1; }

void sigpipe_handler() { signal(SIGPIPE, sigpipe_handler); }



/*
frees all the memory allocated
@param **head head of linked list
*/
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


/*
duplicates a string
@param *src string to be duplicated
@return pointer to a null-terminated string, which is a duplicate of the *src string
*/
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


/*
parses the next line from the database
@param fd file descriptor of database
@param buf buffer to store the information of the line
@param len length of line
@return 0 and line length if parsed successfully, 1 otherwise
*/
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


/*
parses the database
@param head head of linked list
@return 0 if parsed successfully, 1 otherwise
*/
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


/* deals with one client in one thread, reading their requests and responding to them */
void *handle_client(void *client_args) {
    Client *c;
    ServerClient *args = (ServerClient *)client_args;

	sigset_t mask; // represents a set of signals to block in this thread
	sigemptyset(&mask); // initialize the set to an empty set
	sigaddset(&mask, SIGINT); // add the SIGUSR1 signal to the set
	pthread_sigmask(SIG_BLOCK, &mask, NULL); // block the SIGUSR1 signal in this thread

    printf("thread lauched\n");

    while (1) {
        if (args == NULL) { continue; }

        pthread_mutex_lock((*args).mtx);

        while (*((*args).count) == 0) {
            printf("thread(unknown) will wait, buffer empty!\n");
            pthread_cond_wait((*args).consumer_cond, (*args).mtx);
        }

        printf("thread will proceed, buffer not empty!\n");

        c = (*args).client[*((*args).consumer_ptr)]; /* retrieve client */
        
        *((*args).consumer_ptr) += 1;
        if (*((*args).consumer_ptr) == MAX_CLIENTS) { *((*args).consumer_ptr) = 0; }
    
        *((*args).count) -= 1;

        pthread_cond_signal((*args).producer_cond);

        pthread_mutex_unlock((*args).mtx);

        if (c == NULL) { continue; }
        else {
            int n, request_fifo_fd, response_fifo_fd;
            QuestionNode *node = *((*args).head);

            printf("thread identified <%s><%s><%d> and will start!\n", (*c).request_fifo_path, (*c).response_fifo_path, (*c).id);

            request_fifo_fd = open((*c).request_fifo_path, O_RDONLY);
            response_fifo_fd = open((*c).response_fifo_path, O_WRONLY);

            free((*c).request_fifo_path);
            free((*c).response_fifo_path);
            free(c);

            while (node != NULL) {
                char c;
                int next_question = 0;
                int done = 0;

                /* 1. the server starts by writing the status of the question, i.e.,
                        if we are not in the last question (PROCEED), or
                        if we are in the last question (LAST_QUESTION), or exceptionally,
                        if the server has to terminate because of SIGINT (DISCARD)
                */
                if ((*node).next == NULL) { c = LAST_QUESTION; }
                else { c = PROCEED; }

                n = write(response_fifo_fd, &c, 1); /* if client has finished, a sigpipe will be throwed */
                
                if (n == -1) {
                    printf("write error: the response fifo appears to be broken. is the client finished?\n");
                    break;
                }

                while ((n = read(request_fifo_fd, &c, 1)) == 1) { /* 2. server reads the client requests for this question */
                    switch (c) {
                        case QUESTION:
                            write(response_fifo_fd, (*(*node).question).question, (*(*node).question).question_length + 1);
                            break;
                        
                        case ANSWER:
                            write(response_fifo_fd, (*(*node).question).answer, (*(*node).question).answer_length + 1);
                            break;
                        
                        case CLUE:
                            write(response_fifo_fd, (*(*node).question).clue, (*(*node).question).clue_length + 1);
                            break;
                        
                        case NEXT_QUESTION:
                            next_question = 1; /* so we can exit the inner loop that reads customer requests, and move on to the next question */
                            break;
                        
                        case EXIT: 
                            next_question = 1;
                            done = 1;
                            break;; /* the client has finished */
                    }
                    if (next_question) { break; } /* exit the inner loop */
                }
                if (done) { break; }
                if (n == 0) { /* if the client has finished! */
                    printf("read error: the request fifo appears to be broken. is the client finished?\n");
                    break;
                }
                node = (*node).next;
            }
            printf("thread has finished one client\n");
            close(request_fifo_fd);
            close(response_fifo_fd);
        }
    }
}





int main(int argc, char **argv) {
    char *request_fifo_path, *response_fifo_path;
    int i, register_fifo_fd, len, producer_ptr = 0, consumer_ptr = 0, count = 0, status = 0;
    QuestionNode *linked_list_questions_head = NULL;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t producer_cond = PTHREAD_COND_INITIALIZER;
	pthread_cond_t consumer_cond = PTHREAD_COND_INITIALIZER;
    pthread_t threads[MAX_CLIENTS];
    Client *buffer[MAX_CLIENTS];
    ServerClient *common_arguments;


    if (argc != 2) {
        printf("usage: %s <register-fifo>\n", argv[0]);
        return 1;
    }

    if (mkfifo(argv[1], 0666) == -1) {
        printf("failed to create register fifo\n");
        return 1;
    }

    if ((register_fifo_fd = open(argv[1], O_RDONLY)) == -1) {
        printf("failed to open the register fifo\n");
        return 1;
    }

    if (parser(&linked_list_questions_head)) {
        printf("failed to parse the database\n");
        return 1;
    }
    printf("database was parsed successfully\n");

    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, sigpipe_handler);

    common_arguments = malloc(sizeof(ServerClient));
    (*common_arguments).status = &status;
    (*common_arguments).count = &count;
    (*common_arguments).producer_ptr = &producer_ptr;
    (*common_arguments).producer_cond = &producer_cond;
    (*common_arguments).consumer_ptr = &consumer_ptr;
    (*common_arguments).consumer_cond = &consumer_cond;
    (*common_arguments).mtx = &mutex;
    (*common_arguments).client = buffer;
    (*common_arguments).head = &linked_list_questions_head;

    for (i = 0; i < MAX_CLIENTS; i++) { pthread_create(&threads[i], NULL, handle_client, common_arguments); }

    while (1) {
        if (quit) { break; }
        if((read(register_fifo_fd, &len, sizeof(int))) == sizeof(int)) {
            Client *new_client = malloc(sizeof(Client));

            if (len == 11) {
                printf("received code from client to terminate server\n");
                free(new_client);
                break;
            }

            printf("registering client...\n");

            request_fifo_path = malloc(len + 1);
            read(register_fifo_fd, request_fifo_path, len);
            request_fifo_path[len] = '\0';

            read(register_fifo_fd, &len, sizeof(int));
            response_fifo_path = malloc(len + 1);
            read(register_fifo_fd, response_fifo_path, len);
            response_fifo_path[len] = '\0';

            (*new_client).request_fifo_path = request_fifo_path;
            (*new_client).response_fifo_path = response_fifo_path;

            pthread_mutex_lock(&mutex);

            while (count == MAX_CLIENTS) {
                printf("server is full at the moment, please wait!\n");
                pthread_cond_wait(&producer_cond, &mutex);
            }

            (*new_client).id = producer_ptr;

            buffer[producer_ptr] = new_client;
            printf("client registered! <%s><%s><%d>\n", request_fifo_path, response_fifo_path, producer_ptr);

            producer_ptr++;
            if (producer_ptr == MAX_CLIENTS) { producer_ptr = 0; }

            count++;

            pthread_cond_signal(&consumer_cond);

            pthread_mutex_unlock(&mutex);
        }
    }

    status = 1;

    /*
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&producer_cond);
    pthread_cond_destroy(&consumer_cond);
    */
    

    free(common_arguments);

    clear(&linked_list_questions_head);
    close(register_fifo_fd);
    unlink(argv[1]);

    return 0;
}