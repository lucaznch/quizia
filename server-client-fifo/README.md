# Server-client program for a single client
### Compilation
To compile the server and client programs, use the provided **Makefile**.

### Running the Program
1. Start the server in one terminal:
```sh
./server <register-fifo-path>
```
2. In another terminal, start the client:

```sh
./client <register-fifo-path> <request-fifo-path> <response-fifo-path>
```