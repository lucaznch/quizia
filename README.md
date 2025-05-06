# Potential Acetylsalicylic

## Quizia - A simple, fun and challenging trivia quiz
### How to play?
* The user will start with a total of 2 points.
* One point is added for answering a question correctly.
* One point is deducted for failing a question.
* Upon reaching a negative score, the game is immediately over.
* Try to get as many points as possible.
### Input
- **Getting helpful information**: type the `/help` command.
- **Starting the game**: type the `/start` command.
- **Answering**: Simply type your answer and press enter.
- **Requesting a clue**: Type the `/clue` command.
- **Requesting your current points**: Type the `/points` command.
- **Exiting**: Type the `/exit` command.

### Programs
In total I made 4 programs, listed here in chronological order:
1. [here](base/) you can find the base program of a basic trivia quiz game.
2. [here](server-client-fifo/) you can find a server-client program, using named pipes (FIFOs) for inter-process communication, for a **single** client.
3. [here](big-server-client-fifo/) you can find a server-client program, using named pipes (FIFOs) for inter-process communication, for **multiple** clients.
4. [here](server-client-socket/) you can find a server-client program, using TCP/IP sockets for inter-process communication, for a **single** client.
