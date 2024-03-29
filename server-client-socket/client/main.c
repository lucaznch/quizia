#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // 127.0.0.1 for local machine. change this to the server's IP address

#define BUF_PRS_SIZE 84
#define BUF_CMD_SIZE 16
#define BUF_ANS_SIZE 32

#define CMD_START 2
#define CMD_EXIT 3
#define CMD_HELP 4
#define CMD_POINTS 5
#define CMD_CLUE 6
#define CMD_INVALID 7
#define CMD_EOC 8
#define CMD_NOT 9

#define QUESTION 'q'
#define ANSWER 'a'
#define CLUE 'c'
#define NEXT_QUESTION 'n'
#define EXIT 'e'
#define LAST_QUESTION 'l'
#define PROCEED 'p'
#define DISCARD 'd'

#define CORRECT "\t\t\t\033[1;32mCorrect Answer!\033[0m\n\n"
#define INCORRECT "\t\t\t\033[1;31mWrong Answer!\033[0m\n\n"
#define LOSE "\033[1;31mYou have -1 points! YOU LOST!\033[0m\n"
#define WIN "\033[1;32mCongratulations! You answered all the questions!\033[0m\n"
#define SCREEN_HOME "\n\t\tWelcome to Quizia - a simple, fun and challenging trivia quiz\n\n\n"
#define HELP    "\t\t\033[1;33mHOW TO PLAY?\033[0m\n"\
                "\t- You will start with a total of 2 points\n"\
                "\t- One point is added for answering a question correctly\n"\
                "\t- One point is deducted for failing a question\n"\
                "\t- Upon reaching a negative score, the game is immediately over\n"\
                "\t- Try to get as many points as possible\n\n"\
                "\t\t\033[1;33mINPUT?\033[0m\n"\
                "\t- Getting helpful information: type the /help command\n"\
                "\t- Starting the game: type the /start command\n"\
                "\t- Answering: Simply type your answer and press enter\n"\
                "\t- Requesting a clue: Type the /clue command\n"\
                "\t- Requesting your points: Type the /points command\n"\
                "\t- Exiting: Type the `/exit` command\n\n"



/// @brief concatenate two strings
/// @param src initial string
/// @param plus string to concatenate
/// @param len length of the concatenated string
/// @return pointer to the concatenated string and its length
char *concatenate(char *src, char *plus, int *len) {
    char *str, *p;
    int len_src = 0, len_plus = 0;

    while (src[len_src]) { len_src++; }
    while (plus[len_plus]) { len_plus++; }
    *len = len_src + len_plus;
    str = malloc(*len + 1);
    p = str;
    while (*src) { *(p++) = *(src++); }
    while (*plus) {  *(p++) = *(plus++); }
    *p = '\0';

    return str;
}


/// @brief reads until a newline character is found. Note that the newline character is read, and it's suggested before calling this function to check if the current byte, before calling this function, contains the newline
/// @param fd file descriptor to read from
void cleanup(int fd) {
  char ch;
  while (read(fd, &ch, 1) == 1 && ch != '\n') { ; }
}


/// @brief reads a line and returns the corresponding command. If len and answer are valid (!= 0), the function expects to read an answer and not a command and returns the string of the answer
/// @param fd file descriptor to read from
/// @param len length of the answer
/// @param answer answer
/// @return the type of command read and the answer if it is an answer
int get_command(int fd, int len, char *answer) {
    char buf[BUF_CMD_SIZE];
    int n;

    if (read(fd, buf, 1) != 1) { return CMD_EOC; }

    if (buf[0] != '/') { /* not a command */
        if (len == 0 && answer == NULL) {
            if (buf[0] != '\n') { cleanup(fd); } /* verify if last byte read is a newline character before calling cleanup(), that is, if there is more to read */
            return CMD_INVALID;
        }
        else {
            int i = 1;
            len--; /* we already read the first char of answer */
            answer[0] = buf[0];
        
            while (len-- > 0 && read(fd, answer + i, 1) == 1) {
                if (answer[i] == '\n') { break; }
                if (len == 0) { /* if we reach the end of answer size and no '\n' is found, cleanup the line */
                    cleanup(fd);
                    break;    
                } 
                i++;
            }
            answer[i] = '\0';

            return CMD_NOT;
        }
    }

    if (read(fd, buf + 1, 1) != 1) { return CMD_INVALID; }

    /* buf[1] is the command identifier such that: s = /start, e = /exit, h = /help, p = /points, c = /clue */
    switch (buf[1]) {
        case 's': 
            if ((n = read(fd, buf + 2, 4)) != 4 || strncmp(buf, "/start", 6) != 0) { /* read the next 4 bytes expecting to form the string "tart", and store them in the buffer and check if it matches the command */
                if (n == 4 && buf[5] != '\n') { cleanup(fd); } /* verify if last byte read is a newline character before calling cleanup(), that is, if there is more to read */
                return CMD_INVALID;
            }
            if (read(fd, buf + 6, 1) != 0 && buf[6] != '\n') { /* read the next single byte expecting to find the newline character */
                cleanup(fd);
                return CMD_INVALID;
            }
            return CMD_START;
        case 'e':
            if ((n = read(fd, buf + 2, 3)) != 3 || strncmp(buf, "/exit", 5) != 0) {
                if (n == 3 && buf[4] != '\n') { cleanup(fd); }
                return CMD_INVALID;
            }
            if (read(fd, buf + 5, 1) != 0 && buf[5] != '\n') {
                cleanup(fd);
                return CMD_INVALID;
            }
            return CMD_EXIT;
        case 'h':
            if ((n = read(fd, buf + 2, 3)) != 3 || strncmp(buf, "/help", 5) != 0) {
                if (n == 3 && buf[4] != '\n') { cleanup(fd); }
                return CMD_INVALID;
            }
            if (read(fd, buf + 5, 1) != 0 && buf[5] != '\n') {
                cleanup(fd);
                return CMD_INVALID;
            }
            return CMD_HELP;
        case 'p':
            if ((n = read(fd, buf + 2, 5)) != 5 || strncmp(buf, "/points", 7) != 0) {
                if (n == 5 && buf[6] != '\n') { cleanup(fd); }
                return CMD_INVALID;
            }
            if (read(fd, buf + 7, 1) != 0 && buf[7] != '\n') {
                cleanup(fd);
                return CMD_INVALID;
            }
            return CMD_POINTS;
        case 'c':
            if ((n = read(fd, buf + 2, 3)) != 3 || strncmp(buf, "/clue", 5) != 0) {
                if (n == 3 && buf[4] != '\n') { cleanup(fd); }
                return CMD_INVALID;
            }
            if (read(fd, buf + 5, 1) != 0 && buf[5] != '\n') {
                cleanup(fd);
                return CMD_INVALID;
            }
            return CMD_CLUE;
        case '\n':
            return CMD_INVALID;
        default:
            cleanup(fd);
            return CMD_INVALID;  
    } 
}


/// @brief converts a letter to lowercase if it is an uppercase letter
/// @param c integer representing a characther
/// @return letter in lowercase
int lower(char c) {
    if (c >= 'A' && c <= 'Z') { return c + ('a' - 'A'); }
    return c;
}


/// @brief compares the user's answer with the correct answer with the addition of letting a single error pass if there is one
/// @param correct_answer correct answer
/// @param answer user's answer
/// @return 0 if the answers are the same, 1 otherwise
int compare(char *correct_answer, char *answer) {
    int diff, mistakes = 0;

    /* while pointers are different than '\0' */
    while (*correct_answer && *answer) {
        diff = lower(*correct_answer) - lower(*answer); 

        if (diff != 0) {
            mistakes++; 
            if (mistakes > 1) { return diff; }
        }

        correct_answer++;
        answer++;
    }
    return lower(*correct_answer) - lower(*answer);
}


/// @brief responsible for the game
/// @param client_socket_fd descriptor of the client socket
void game(int client_socket_fd) {
    char request, question_status, user_buf[BUF_ANS_SIZE], server_buf[BUF_PRS_SIZE];
    int answered, points = 2;

    memset(server_buf, '\0', BUF_PRS_SIZE);
    printf("\n\n");

    while (1) {
        answered = 0; // tells if the user has answered to the question

        // 1. the client starts by reading the status of the question, i.e.,
        //      if we are not in the last question (PROCEED), or
        //      if we are in the last question (LAST_QUESTION), or exceptionally,
        //      if the server has to terminate because of SIGINT (DISCARD)

        read(client_socket_fd, &question_status, 1);

        if (question_status == DISCARD) { // if the server has to terminate, we also terminate the client
            printf("server terminated\n");
            return;
        }
	
	    system("clear");

        request = QUESTION;
        send(client_socket_fd, &request, 1, 0); // 2. if status of question is ok, the client requests the question
        read(client_socket_fd, server_buf, BUF_PRS_SIZE); // 3. the client reads the question and stores it
        printf("%s\n", server_buf); // print the question

        while (!answered) {
            printf("> ");
            fflush(stdout);
            switch (get_command(STDIN_FILENO, BUF_ANS_SIZE, user_buf)) { // get user's command from stdin
                case CMD_EXIT:
                    request = EXIT;
                    send(client_socket_fd, &request, 1, 0); // the client "requests" its termination
                    return; // the client terminates
                case CMD_HELP:
                    printf(HELP);
                    break;
                case CMD_POINTS:
                    printf("you have a total of %d points\n", points);
                    break;
                case CMD_CLUE:
                    request = CLUE;
                    send(client_socket_fd, &request, 1, 0); // the client requests a clue
                    read(client_socket_fd, server_buf, BUF_PRS_SIZE); // the client reads the clue and stores it
                    printf("%s\n", server_buf); // print the clue
                    break;
                case CMD_INVALID:
                    printf("invalid command!\n");
                    break;
                case CMD_NOT:
                    answered = 1;
                    request = ANSWER;
                    send(client_socket_fd, &request, 1, 0); // the client requests the answer
                    read(client_socket_fd, server_buf, BUF_PRS_SIZE); // the client reads the answer and stores it

                    if (compare(server_buf, user_buf) == 0) {
                        printf(CORRECT);
                        sleep(1);
                        points++;
                    }
                    else {
                        printf(INCORRECT);
                        sleep(1);
                        points--;
                        if (points < 0) {
                            printf(LOSE);
                            request = EXIT;
                            send(client_socket_fd, &request, 1, 0); // the client "requests" its termination
                            return;
                        }
                    }
                    if (question_status == LAST_QUESTION) {
                        printf(WIN);
                        request = EXIT;
                        send(client_socket_fd, &request, 1, 0); // the client "requests" its termination
                        return;
                    }
                    break;
                default:
                    break;
            }
        }
        request = NEXT_QUESTION;
        send(client_socket_fd, &request, 1, 0);
    }
}


int main() {
    int client_socket_fd = 0, stop = 0;
    struct sockaddr_in serv_addr;

    // creates the client socket of type SOCK_STREAM, domain AF_INET (IPv4), protocol 0 (default)
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("failed to create client socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("invalid address / address not supported\n");
        return -1;
    }

    if (connect(client_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("connection failed \n");
        return -1;
    }

    system("clear"); 

    printf(SCREEN_HOME);
    while (1) {
        if (stop) { break; }

        printf("> ");
        fflush(stdout);
        switch (get_command(STDIN_FILENO, 0, NULL)) { /* get user's command from stdin */
            case CMD_START:
                game(client_socket_fd);
                stop = 1;
                break;
            case CMD_EXIT:
                stop = 1;
                break;
            case CMD_HELP:
                printf(HELP);
                break;
            case CMD_INVALID:
                printf("invalid command!\n");
                break;
            default:
                break;
        }
    }
    close(client_socket_fd);
    return 0;
}
