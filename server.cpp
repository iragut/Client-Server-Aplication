#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <vector>

#include "utils-common-func.hpp"

using namespace std;

// Find the id of the client with the given socket
string find_id(int sockfd, map<string, int> server_clients) {
    for (auto client_id = server_clients.begin(); client_id != server_clients.end(); client_id++) {
        if (client_id->second == sockfd)
            return client_id->first;
    }
    return NULL;
}

// Copy the content from the udp packet to the tcp packet
void content_populate(udp_header *udp_packet, packet *tcp_packet, int size) {
    tcp_packet->content = new char[size];
    memcpy(tcp_packet->content, udp_packet->content, size);
    tcp_packet->head_packet.size = size;
}

// Create and send a tcp packet to the client
void create_and_send_tcp_packet(int sockfd, udp_header* udp_packet, struct sockaddr_in cli_addr) {
    packet *tcp_packet = new packet;
    memset(tcp_packet, 0, sizeof(packet));

    tcp_packet->head_packet.port = ntohs(cli_addr.sin_port);
    tcp_packet->head_packet.ip = cli_addr.sin_addr.s_addr;
    tcp_packet->head_packet.data_type = udp_packet->data_type;
    
    strcpy(tcp_packet->head_packet.topic, udp_packet->topic);

    // Create dynamica content for the tcp packet
    switch (udp_packet->data_type) {
        case 0:
            content_populate(udp_packet, tcp_packet, INT_CONTENT_LEN);
            break;
        case 1:
            content_populate(udp_packet, tcp_packet, SHORT_REAL_CONTENT_LEN);
            break;
        case 2:
            content_populate(udp_packet, tcp_packet, FLOAT_CONTENT_LEN);
            break;
        case 3:
            content_populate(udp_packet, tcp_packet, strlen(udp_packet->content) + 1);
            break;
    }

    send_all(sockfd, (char *)&tcp_packet->head_packet, sizeof(header));
    send_all(sockfd, (char *)tcp_packet->content, tcp_packet->head_packet.size);

    delete tcp_packet->content;
    delete tcp_packet;
}

// Check if the given topic is subscribed to the given sub_topic
bool is_subscribed(string topic, string sub_topic) {

    char topic_cpy[MAX_TOPIC_LEN];
    char sub_topic_cpy[MAX_TOPIC_LEN];

    vector<string> topic_parts;
    vector<string> sub_topic_parts;

    // If the topic is the same as the sub_topic or is only the wildcard "*"
    if (sub_topic == "*" || topic == sub_topic)
        return true;

    // Create a vector with every parts of the topic and sub_topic
    strcpy(topic_cpy, topic.c_str());
    strcpy(sub_topic_cpy, sub_topic.c_str());

    char *token = strtok(topic_cpy, "/");
    while (token != NULL) {
        topic_parts.push_back(token);
        token = strtok(NULL, "/");
    }

    char *sub_token = strtok(sub_topic_cpy, "/");
    while (sub_token != NULL) {
        sub_topic_parts.push_back(sub_token);
        sub_token = strtok(NULL, "/");
    }
    
    size_t i = 0, j = 0, k = 0;

    // Check if the topic is included in the sub_topic
    while (i < topic_parts.size() && j < sub_topic_parts.size()) {
        if (sub_topic_parts[j] == "+") {
            if (j == sub_topic_parts.size() - 1 && i < topic_parts.size() - 2)
                return false;
            j++; i++;

        } else if (sub_topic_parts[j] == "*") {
            if (j == sub_topic_parts.size() - 1)
                return true;

            string next = sub_topic_parts[j + 1];

            for (k = i; k < topic_parts.size(); k++) {
                if (topic_parts[k] == next) {
                    i = k;
                    break;
                }
            }

            if (k == topic_parts.size())
                return false;

            j += 2;
        } else if (topic_parts[i] != sub_topic_parts[j]) {
            return false;
        } else {
            i++; j++;
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 2, "Usage: ./server <PORT>");

    int i, nfds = 0;
    char buffer[BUFLEN];
    struct pollfd fds[MAX_CONNECTIONS];

    // Map for store the connected clients, and they subscribed topics
    map<string, int> server_clients;
    map<string, vector<string>> server_topics;

    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");
    
    // Create the tcp and udp sockets and add them to the poll
    int tcp_socket = create_socket(port, SOCK_STREAM, false);
    int udp_socket = create_socket(port, SOCK_DGRAM, false);

    nfds = add_in_poll(fds, nfds, STDIN_FILENO);
    nfds = add_in_poll(fds, nfds, tcp_socket);
    nfds = add_in_poll(fds, nfds, udp_socket);

    while(true) {
        poll(fds, nfds, -1);
        for (i = 0; i < nfds; i++) {
            // If the command is exit close the connections and exit the program
            if (fds[i].revents & POLLIN && fds[i].fd == STDIN_FILENO) {
                cin.getline(buffer, sizeof(buffer));
                if (strncmp(buffer, "exit", 4) == 0){
                    tcp_close_connection(tcp_socket);
                    tcp_close_connection(udp_socket);
                    return 0;
                } else {
                    cout << "Commands accepted: exit\n";
                }
            // If the event is from a tcp socket when is a new or an existing client
            // who wants to connect
            } else if (fds[i].revents & POLLIN && fds[i].fd == tcp_socket) {
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);

                int newsockfd = accept(tcp_socket, (struct sockaddr *)&cli_addr, &cli_len);
                DIE(newsockfd < 0, "accept");

                // Receive the id of the client
                rc = recv(newsockfd, buffer, sizeof(buffer), 0);
                DIE(rc < 0, "recv");

                // Check if the client is already connected or not
                if (server_clients.find(buffer) != server_clients.end()) {
                    cout << "Client " << buffer << " already connected.\n";

                    tcp_close_connection(newsockfd);
                } else {                
                    cout << "New client " << buffer 
                    << " connected from " << inet_ntoa(cli_addr.sin_addr) 
                    << ":" << ntohs(cli_addr.sin_port) << ".\n";

                    nfds = add_in_poll(fds, nfds, newsockfd);
                    server_clients[buffer] = newsockfd;
                }
            // If the event is from the udp socket then we receive the udp packet and
            // need to check if the topic is subscribed by any client
            } else if ((fds[i].revents & POLLIN) && (fds[i].fd == udp_socket)) {
                udp_header *udp_packet = new udp_header;
                memset(udp_packet, 0, sizeof(udp_header));

                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);

                rc = recvfrom(udp_socket, udp_packet, sizeof(udp_header), 0, (struct sockaddr *)&cli_addr, &cli_len);
                DIE(rc < 0, "recvfrom");

                string topic = udp_packet->topic;
                
                // Check if the topic is subscribed by any client
                for (auto id_client = server_topics.begin(); id_client != server_topics.end(); id_client++) {
                    for (auto sub_topic = id_client->second.begin(); sub_topic != id_client->second.end(); sub_topic++) {
                        string id = id_client->first;

                        // If the topic is subscribed by the client then send the tcp packet
                        if (is_subscribed(topic, *sub_topic) && (server_clients.find(id) != server_clients.end())) {
                            create_and_send_tcp_packet(server_clients[id], udp_packet, cli_addr);
                            delete udp_packet;
                            break;
                        }
                    }
                }
            
            // If the event is not from the udp or tcp socket then is from a client
            } else if ((fds[i].revents & POLLIN) && (i > 2)) {
                char cmd[MAX_CMD_LEN];
                rc = recv_all(fds[i].fd, cmd, sizeof(cmd));

                // Check what comand the client sent
                strtok(cmd, " ");
                char *topic = strtok(NULL, " ");
                // If the rc is 0 the client disconnected so we need to close the connection
                // and remove it from the poll
                if (rc == 0) {

                    for (auto client_socked = server_clients.begin(); client_socked != server_clients.end(); client_socked++) {
                        if (client_socked->second == fds[i].fd) {
                            cout << "Client " << client_socked->first << " disconnected.\n";
                            server_clients.erase(client_socked);
                            break;
                        }
                    }

                    for (int j = i; j < nfds - 1; j++)
                        fds[j] = fds[j + 1];
                    nfds--;

                    tcp_close_connection(fds[i].fd);

                // If the command is subscribe or unsubscribe we add or remove the topic from or map
                } else if (strncmp(cmd, "subscribe", 9) == 0) {
                    string id = find_id(fds[i].fd, server_clients);
                    server_topics[id].push_back(topic);

                } else if (strncmp(cmd, "unsubscribe", 11) == 0) {
                    string id = find_id(fds[i].fd, server_clients);
                    for (auto sub_topic = server_topics[id].begin(); sub_topic != server_topics[id].end(); sub_topic++) {
                        if (*sub_topic == topic) {
                            server_topics[id].erase(sub_topic);
                            break;
                        }
                    }
                }
            }
        }
    }

    return 0;
}