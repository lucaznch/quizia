# Quizia - a simple, fun and challenging trivia quiz
## How to play?
* The client will start with a total of 2 points.
* One point is added for answering a question correctly.
* One point is deducted for failing a question.
* Upon reaching a negative score, the game is immediately over.
* Try to get as many points as possible.
## Game Commands
- **Getting helpful information**: type the `/help` command.
- **Starting the game**: type the `/start` command.
- **Answering**: Simply type your answer and press enter.
- **Requesting a clue**: Type the `/clue` command.
- **Requesting your points**: Type the `/points` command.
- **Exiting**: Type the `/exit` command.
### Overview
This project is a server-client based trivia quiz game implemented in C, utilizing named pipes (FIFOs) for inter-process communication. The game allows a single client to connect to the server. The server holds a database of quiz questions, each with a corresponding answer and clue, that is parsed into a linked list.
### Communication
Communication between the client and the server is established through named pipes (FIFOs):
- **Server Side**: The server listens on a read-only named pipe, `register fifo`, for a registration request from the client process. A registration request includes two integers indicating the sizes (in bytes) of two strings specifying the pathnames of the client's named pipes (*request_fifo* and *response_fifo*), such that: `<size1><request-fifo-path><size2><response-fifo-path>`.
- **Client Side**: The client manages two named pipes: 
  - `request fifo` for sending requests to the server (write-only).
  - `response fifo` for receiving responses from the server (read-only).
### Compilation
To compile the server and client programs, either use the provided **Makefile** or use the following commands:

```sh
gcc -Wall -Werror -Wextra -o server server.c
gcc -Wall -Werror -Wextra -o client client.c
```
### Running the Program
1. Start the server in one terminal:
```sh
./server <register-fifo-path>
```
2. In another terminal, start the client:

```sh
./client <register-fifo-path> <request-fifo-path> <response-fifo-path>
```