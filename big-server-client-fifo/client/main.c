#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

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



/*
concatenates two strings
@param src initial string
@param plus string to concatenate
@param len length of the concatenated string
@return pointer to the concatenated string and its length
*/
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


/*
reads until a newline character is found, 
the newline character is read, 
it's suggested before calling this function to check if the previous byte contains the newline
@param fd file descriptor to read from
*/
void cleanup(int fd) {
  char ch;
  while (read(fd, &ch, 1) == 1 && ch != '\n') { ; }
}


/*
reads a line and returns the corresponding command. if len and *answer are valid the function returns the "command" string, i.e., the answer of a question!
@param fd file descriptor to read from
@param len length of the answer
@param answer answer
@return the command read
*/
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


/*
converts a letter to lowercase if it is an uppercase letter
@param c integer representing a characther
@return letter in lowercase
*/
int lower(char c) {
    if (c >= 'A' && c <= 'Z') { return c + ('a' - 'A'); }
    return c;
}


/*
compares the user's answer with the correct answer with the addition of letting a single error pass if there is one
@param correct_answer correct answer
@param answer user's answer
@return 0 if the answers are the same, 1 otherwise
*/
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


/*
evaluates the user's answer
@param points total of points
@param request_fifo_fd file descriptor of the request fifo
@param response_fifo_fd file descriptor of the response fifo
@param request should be ANSWER
@param question_status status of the question retrieved from the server
@param user_buf buffer that has the user's answer
@param server_buf buffer that has the server's answer
@return 0 if the answer was correct and it wasn't the last question OR if the answer's incorrect and it was not the last question and did not cause the score to be negative
@return 1 if the answer was incorrect and the score became negative OR it was the last question
*/
int evaluate_answer(int *points, int request_fifo_fd, int response_fifo_fd, char request, char question_status, char *user_buf, char *server_buf) {
    
    write(request_fifo_fd, &request, 1); /* the client requests the answer */
    read(response_fifo_fd, server_buf, BUF_PRS_SIZE); /* the client reads the answer and stores it */

    if (compare(server_buf, user_buf) == 0) { /* if the user's answer is the same as the server's answer */
        printf(CORRECT);
        (*points)++;
    }
    else { 
        printf(INCORRECT);
        (*points)--;
        if ((*points) == -1) { /* if we reach a negative score */
            printf(LOSE);
            request = EXIT;
            write(request_fifo_fd, &request, 1); /* the client "requests" its termination */
            return 1;
        }
    }
    if (question_status == LAST_QUESTION) { /* if this is the last question and the user "survived" */
        request = EXIT;
        write(request_fifo_fd, &request, 1); /* the client "requests" its termination */
        printf(WIN);
        return 1;
    }
    return 0;
}





/*
responsible for the entire trivia quiz game
@param request_fifo_fd file descriptor of the request fifo
@param response_fifo_fd file descriptor of the response fifo
*/
void game(int request_fifo_fd, int response_fifo_fd) {
    char request, question_status, user_buf[BUF_ANS_SIZE], server_buf[BUF_PRS_SIZE];
    int answered, points = 2;

    memset(server_buf, '\0', BUF_PRS_SIZE);
    printf("\n\n");

    while (1) {
        answered = 0; /* variable that tells if the user has answered to the question */
        
        /* 1. the client starts by reading the status of the question, i.e.,
                if we are not in the last question (PROCEED), or
                if we are in the last question (LAST_QUESTION), or exceptionally,
                if the server has to terminate because of SIGINT (DISCARD)
        */
        read(response_fifo_fd, &question_status, 1);

        if (question_status == DISCARD) { return; } /* if the server has to terminate, we also terminate the client  */

        request = QUESTION;
        write(request_fifo_fd, &request, 1); /* 2. if status of question is ok, the client requests the question */
        read(response_fifo_fd, server_buf, BUF_PRS_SIZE); /* 3. the client reads the question and stores it */
        printf("%s\n", server_buf); /* print the question */

        while (!answered) { /* while the use has not yet answered to the question */
            printf("> ");
            fflush(stdout);
            switch (get_command(STDIN_FILENO, BUF_ANS_SIZE, user_buf)) { /* get user's command from stdin */
                case CMD_EXIT:
                    request = EXIT;
                    write(request_fifo_fd, &request, 1); /* the client "requests" its termination */
                    return; /* the client terminates */
                case CMD_HELP:
                    printf(HELP);
                    break;
                case CMD_POINTS:
                    printf("you have a total of %d points\n", points);
                    break;
                case CMD_CLUE:
                    request = CLUE;
                    write(request_fifo_fd, &request, 1); /* the client requests a clue */
                    read(response_fifo_fd, server_buf, BUF_PRS_SIZE); /* the client reads the clue and stores it */
                    printf("%s\n", server_buf); /* print the clue */
                    break;
                case CMD_INVALID:
                    printf("invalid command!\n");
                    break;
                case CMD_NOT:
                    answered = 1;

                    /* if the answer was incorrect and the score became negative OR it was the last question */
                    if (evaluate_answer(&points, request_fifo_fd, response_fifo_fd, ANSWER, question_status, user_buf, server_buf)) { return; }
                    break;
                default:
                    break;
            }
        }

        request = NEXT_QUESTION;
        write(request_fifo_fd, &request, 1);
    }
}


/*
@param register_fifo_fd file descriptor of the register fifo
@param request_fifo_fd file descriptor of the request fifo
@param response_fifo_fd file descriptor of the response fifo
@param request_fifo_path path of the request fifo
@param response_fifo_path path of the response fifo
*/
void close_files(int register_fifo_fd, int request_fifo_fd, int response_fifo_fd, char *request_fifo_path, char *response_fifo_path) {
    close(register_fifo_fd);
    close(request_fifo_fd);
    close(response_fifo_fd);
    unlink(request_fifo_path);
    unlink(response_fifo_path);
}


int main(int argc, char **argv) {
    char *request_fifo_path, *response_fifo_path;
    int register_fifo_fd, request_fifo_path_len, response_fifo_path_len, request_fifo_fd, response_fifo_fd, stop = 0;

    if (argc != 4) {
        printf("usage: %s <register-fifo-path> <request-fifo-path> <response-fifo-path>\n", argv[0]);
        return 1;
    }

    if ((register_fifo_fd = open(argv[1], O_WRONLY)) == -1) {
        printf("failed to open server register fifo\n");
        return 1;
    }
    
    if (strlen(argv[2]) == 1 && *argv[2] == 'q') { /* if client is the terminator */
        write(register_fifo_fd, &request_fifo_path_len, sizeof(int)); /* write size of request-fifo-path */
        close(register_fifo_fd);
        return 0;
    }
    
    request_fifo_path = concatenate("../client/", argv[2], &request_fifo_path_len);
    response_fifo_path = concatenate("../client/", argv[3], &response_fifo_path_len);

    write(register_fifo_fd, &request_fifo_path_len, sizeof(int)); /* write size of request-fifo-path */
    write(register_fifo_fd, request_fifo_path, request_fifo_path_len); /* write request-fifo-path */

    write(register_fifo_fd, &response_fifo_path_len, sizeof(int)); /* write size of response-fifo-path */
    write(register_fifo_fd, response_fifo_path, response_fifo_path_len); /* write response-fifo-path */

    free(request_fifo_path);
    free(response_fifo_path);

    if (mkfifo(argv[2], 0660) != 0) {
        printf("failed to create request fifo\n");
        return 1;
    }

    if (mkfifo(argv[3], 0660) != 0) {
        printf("failed to create response fifo\n");
        return 1;
    }

    if ((request_fifo_fd = open(argv[2], O_WRONLY)) == -1) {
        printf("failed to open request fifo\n");
        return 1;
    }

    if ((response_fifo_fd = open(argv[3], O_RDONLY)) == -1) {
        printf("failed to open request fifo\n");
        return 1;
    }

    printf(SCREEN_HOME);
    while (1) {
        if (stop) { break; }

        printf("> ");
        fflush(stdout);
        switch (get_command(STDIN_FILENO, 0, NULL)) { /* get user's command from stdin */
            case CMD_START:
                game(request_fifo_fd, response_fifo_fd);
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

    close_files(register_fifo_fd, request_fifo_fd, response_fifo_fd, argv[2], argv[3]);

    return 0;
}