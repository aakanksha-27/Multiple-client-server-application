// Source: https://book.systemsapproach.org/foundation/software.html
// sudo lsof -i :5432

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define SERVER_PORT  5432
#define MAX_PENDING  5
#define MAX_LINE     256
#define MAX_CLIENTS  1024

void event_loop(int new_s)
{
	fd_set readfds;
	struct timeval tv;
	int max_fd = STDIN_FILENO;
	int activity;
	char buf[MAX_LINE];

	int client[MAX_CLIENTS];

	for (int slot = 0; slot < MAX_CLIENTS; slot++)
		client[slot] = -1;

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(new_s, &readfds);

		if (new_s > max_fd) {
			max_fd = new_s;
		}

		// add all the active clients to the fd set
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (client[i] != -1) {
				FD_SET(client[i], &readfds);
				if (client[i] > max_fd)
					max_fd = client[i];
			}
		}

		tv.tv_sec = 30;
		tv.tv_usec = 0;

		activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

		if (activity < 0) {
			perror("select");
			break;
		} else if (activity == 0) {
			printf("No activity for 30 secs.\n");
			continue;
		}

		// CASE 1: if there is a new connection
		if (FD_ISSET(new_s, &readfds)) {
			struct sockaddr_in client_addr;
			socklen_t addr_len = sizeof(client_addr);
			int new_fd = accept(new_s, (struct sockaddr *)&client_addr, &addr_len);
			if (new_fd < 0) {
				perror("accept failed");
			} else {
				for (int i = 0; i < MAX_CLIENTS; i++) {
					if (client[i] == -1) { // if the slot is empty
						client[i] = new_fd; // give the new fd to that client
						break;
					}
				}
			}
		}

		// CASE 2: send message to a client
		if (FD_ISSET(STDIN_FILENO, &readfds)) {
			// data is available at stdin
			if (fgets(buf, sizeof(buf), stdin) != NULL) {
				buf[MAX_LINE-1] = '\0';

				// take the first active client
				for (int i = 0; i < MAX_CLIENTS; i++) {
					if (client[i] != -1) {
						// and send the content to that client
						if (send(client[i], buf, strlen(buf)+1, 0) < 0) {
							perror("send error");
							close(client[i]);
							client[i] = -1;
						}
						break;
					}
				}
			}
		}

		// CASE 3: print client data to STDOUT
		for (int i = 0; i < MAX_CLIENTS; i++) {
			int fd = client[i];
			if (fd != -1 && FD_ISSET(fd, &readfds)) {
				int bytes = recv(fd, buf, sizeof(buf)-1, 0);
				if (bytes <= 0) {
					printf("client disconnected.\n");
					close(fd); // perform cleanup
					client[i] = -1; // and free the slot
				} else {
					buf[bytes] = '\0';
					printf("client says: %s\n", buf);
				}
			}
		}
	}

	// perform clean up
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (client[i] != -1)
			close(client[i]);
	}
}

int main()
{
	struct sockaddr_in sin;
	// socklen_t addr_len;
	int s;

	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		perror("bind failed");
		exit(1);
	}

	if (listen(s, MAX_PENDING) < 0) {
		perror("listen failed");
		exit(1);
	}

	/*if ((new_s = accept(s, (struct sockaddr *)&sin, &addr_len)) < 0) {
		perror("accept failed");
		exit(1);
	}*/

	event_loop(s);

	close(s);
	return 0;
}
