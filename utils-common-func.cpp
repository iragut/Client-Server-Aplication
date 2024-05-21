#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <poll.h>

using namespace std;
#define MAX_CONNECTIONS 32

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

int tcp_close_connection(int sockfd) {
	int rc;
	rc = close(sockfd);
	DIE(rc < 0, "close");
	return rc;
}

int create_socket(unsigned short port, int flag, bool client) {
	struct sockaddr_in address;
	int listenfd, rc, sock_opt;

	listenfd = socket(PF_INET, flag, 0);
	DIE(listenfd < 0, "socket");

	sock_opt = 1;

	// If is a client we deactivate the Nagle algorithm
	if (client == true)
		rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY,	&sock_opt, sizeof(int));
	else
		rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,	&sock_opt, sizeof(int));

	DIE(rc < 0, "setsockopt");

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;

	// If is for the server we bind the socket else we connect it
	if (client == false) {
		rc = bind(listenfd, (const struct sockaddr *)&address, sizeof(address));
		DIE(rc < 0, "bind");
	}
    if (flag == SOCK_STREAM && client == false) {
        rc = listen(listenfd, MAX_CONNECTIONS);
        DIE(rc < 0, "listen");
    }
	if (client == true) {
		rc = connect(listenfd, (struct sockaddr *)&address, sizeof(address));
		DIE(rc < 0, "connect");
	}

	return listenfd;
}

int recv_all(int sockfd, char *buffer, int size) {
	int bytes_received = 0;
	
	while (size > 0) {
		int rc = recv(sockfd, buffer, size, 0);
		DIE(rc < 0, "recv");

		if (rc == 0)
			return 0;
		size -= rc;
		buffer += rc;
		bytes_received += rc;
	}

	return bytes_received;
}

int send_all(int sockfd, char *buffer, int size) {
	int bytes_sent = 0;

	while (size > 0) {
		int rc = send(sockfd, buffer, size, 0);
		DIE(rc < 0, "send");
		size -= rc;
		buffer += rc;
		bytes_sent += rc;
	}

	return bytes_sent;
}

int add_in_poll(struct pollfd *fds, int nfds, int sockfd) {
	fds[nfds].fd = sockfd;
	fds[nfds].events = POLLIN;
	fds[nfds].revents = 0;
	nfds++;
	return nfds;
}
