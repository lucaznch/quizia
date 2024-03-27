# Quizia - a simple, fun and challenging trivia quiz

### Compilation
To compile the server and client programs, either use the provided **Makefile** or use the following commands:

```sh
gcc -o server server.c
gcc -o client client.c
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