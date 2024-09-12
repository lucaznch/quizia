#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define DATABASE_PATH "super-secret.db"

#define BUF_CMD_SIZE 16
#define BUF_PRS_SIZE 84
#define BUF_ANS_SIZE 32

#define CMD_START 2
#define CMD_EXIT 3
#define CMD_HELP 4
#define CMD_POINTS 5
#define CMD_CLUE 6
#define CMD_INVALID 7
#define CMD_EOC 8
#define CMD_NOT 9

#define HELP "Quizia - a fun trivia quiz!\n"
#define CORRECT "\t\t\t\033[1;32mcorrect answer!\033[0m\n\n"
#define WRONG "\t\t\t\033[1;31mwrong answer!\033[0m\n\n"
#define LOSE "you have -1 points, you lost!\n"
#define GAME_OVER "the game is over!\n\n"
#define TIMES_UP "\033[1;33mTIME IS UP!\033[0m\n"

#define SCREEN_HOME "\n\t\tWelcome to Quizia - a fun and challenging trivia quiz\n\n\n"\
                    "You will start with a total of 2 points\n\n"\
                    "Get a question right and you will earn 1 point\n\n"\
                    "Fail a question and you lose 1 point\n\n"\
                    "If your score becomes negative, the game is over\n\n"\
                    "Try to get as many points as possible!\n\n\n"

#define TO_INT(c) ((c) - '0')

volatile int timer = 0;

typedef struct {
    char *question;
    char *answer;
    char *clue;
} Question;

typedef struct QuestionNode {
    Question *question;
    struct QuestionNode *next;
} QuestionNode;


void sig_alarm_handler() {
    timer = 1; /* change the value, meaning the timer is up */
    signal(SIGALRM, sig_alarm_handler);
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
reads a line and returns the corresponding command - when len == 0 => command. len != 0 => line
@param fd file descriptor to read from
@param len max size of line - used to read a line that is not a command
@param answer stores the line read - used to read a line that is not a command
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


/* auxiliary function */
void traverse(QuestionNode **head) {
    QuestionNode *node = *head;

    while (node != NULL) {
        printf("Question: %s\nAnswer:%s\nClue: %s\n", (*(*node).question).question, (*(*node).question).answer, (*(*node).question).clue);
        node = (*node).next;
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
@param *buf buffer to store the information of the line
@return 0 if parsed successfully, 1 otherwise
*/
int parse_line(int fd, char *buf) {
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

    return 0;
}


/*
parses the database
@param head head of linked list
@return 0 if parsed successfully, 1 otherwise
*/
int parser(QuestionNode **head) {
    int fd;
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
        if (parse_line(fd, buf)) {
            free(q);
            free(node);
            break;
        }
        (*q).question = duplicate(buf);
        
        /* parse line (Answer) */
        parse_line(fd, buf);
        (*q).answer = duplicate(buf);

        /* parse line (Clue) */
        parse_line(fd, buf);
        (*q).clue = duplicate(buf);

        (*node).question = q;

        /* insert node in the beginning of linked list */
        (*node).next = *head;
        *head = node;
    }
    return 0;
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
starts the game
@param total_points total points for user to start with
@param head head of linked list
*/
void start(int total_points, QuestionNode **head) {
    char buf[BUF_ANS_SIZE];
    int answered;
    QuestionNode *node = *head;

    signal(SIGALRM, sig_alarm_handler);    

    while (node != NULL) {
        printf("\n%s\n", (*(*node).question).question);
        
        alarm(15); /* each question has a timer of 15 seconds */

        answered = 0;

        while (!answered) {
            if (timer) {
                printf(TIMES_UP);
                timer = 0;
                total_points--;
                break;
            }
            printf("> ");
            fflush(stdout);
            switch (get_command(STDIN_FILENO, BUF_ANS_SIZE, buf)) {
                case CMD_START:
                    printf("the game has already started!\n");
                    break;
                case CMD_EXIT:
                    printf("leaving the game...\n");
                    sleep(2);
                    printf("done!\n\n");
                    return;
                case CMD_HELP:
                    printf(HELP);
                    break;
                case CMD_POINTS:
                    printf("You have %d points\n", total_points);
                    break;
                case CMD_CLUE:
                    printf("\t%s\n", (*(*node).question).clue);
                    break;
                case CMD_INVALID:
                    printf("invalid command!\n");
                    break;
                case CMD_EOC:
                    /* do nothing */
                    break;
                case CMD_NOT:
                    answered = 1;
                    if (compare((*(*node).question).answer, buf) == 0) {
                        printf(CORRECT);
                        total_points++;
                    }
                    else {
                        printf(WRONG);
                        total_points--;
                        if (total_points == -1) { printf(LOSE); }
                    }
                    break;
            }
        }
        if (total_points < 0) { break; }
        node = (*node).next;
    }
    printf(GAME_OVER);
}


int main() {
    int points = 2;
    QuestionNode *linked_list_questions_head = NULL;

    if (parser(&linked_list_questions_head)) { return 1; }

    printf(SCREEN_HOME);
    while (1) {
        printf("> ");
        fflush(stdout);
        switch (get_command(STDIN_FILENO, 0, NULL)) {
            case CMD_START:
                start(points, &linked_list_questions_head);
                break;
            case CMD_EXIT:
                clear(&linked_list_questions_head);
                return 0;
            case CMD_HELP:
                printf(HELP);
                break;
            case CMD_POINTS:
                printf("You will start with %d points\n", points);
                break;
            case CMD_CLUE:
                /* do nothing */
                break;
            case CMD_INVALID:
                printf("invalid command!\n");
                break;
            case CMD_EOC:
                /* do nothing */
                break;
        }
    }
    return 0;
}
