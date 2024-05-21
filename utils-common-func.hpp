#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils-common-func.cpp"

#define BUFLEN 4096
#define MAX_CMD_LEN 100
#define MAX_TOPIC_LEN 50
#define INT_CONTENT_LEN 5
#define FLOAT_CONTENT_LEN 6
#define MAX_CONTENT_LEN 1500
#define SHORT_REAL_CONTENT_LEN 2
vector<string> types = {"INT", "SHORT_REAL", "FLOAT", "STRING"};

// Packet for udp protocol
typedef struct udp_header {
    char topic[MAX_TOPIC_LEN];
    uint8_t data_type;
    char content[MAX_CONTENT_LEN];
} udp_header;

// Header for tcp protocol
typedef struct header{
    int port;
    int size;
    uint32_t ip;
    uint8_t data_type;
    char topic[MAX_TOPIC_LEN];
} header;

// Packet for tcp protocol
typedef struct packet {
    header head_packet;
    char *content;
} packet;

// Close the connection at the given socket
int tcp_close_connection(int sockfd);
// Receive given size of data from the socket
int recv_all(int sockfd, char *buffer, int size);
// Send given size of data to the socket
int send_all(int sockfd, char *buffer, int size);
// Add a new socket to the poll
int add_in_poll(struct pollfd *fds, int nfds, int sockfd);
// Create a new server or client socket
int create_socket(unsigned short port, int flag, bool client);
