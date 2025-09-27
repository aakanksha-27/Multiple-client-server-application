### THREAD IMPLEMENTATION
- limit of total number of clients in a server session: define MAX_CLIENTS = 1024
- a wrapper method called by the p_thread, which eventually invokes the event_loop for that p_thread because p_thread calls a function of the signature given below: void* event_loop_wrapper(void* arg)
- typecast the argument into the new_s integer: int new_s = (int)(intptr_t)arg
- invoke the event_loop with new_s as the parameter : event_loop(new_s)
- then return null a ptrs
- Now in the int main, tid is an array of p_thread ids of type pthread_t of the size of MAX_CLIENTS: pthread_t *tid = malloc(sizeof(pthread_t) * MAX_CLIENTS)
- initialize the tid of all the client p_threads as 0
- then invoke a loop to accept upto MAX_CLIENTS
- then accept new client connections
- if everything goes well :-) assign a tid to the new client p_thread and invoke the event_loop_wrapper routine with the new_s as parameter
- then wait for all the clients to disconnect in order to stop the current server session

### EVENT IMPLEMENTATION
- CASE 1: if there is a new connection: if (FD_ISSET(new_s, &readfds))
- CASE 2: send message to a client: if (FD_ISSET(STDIN_FILENO, &readfds))
- if data is available at stdin
- take the first active client
- and send the content to that client
- CASE 3: print client data to STDOUT
- then perform clean up
