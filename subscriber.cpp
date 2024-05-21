#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>
#include <math.h>

#include "utils-common-func.hpp"

using namespace std;

// Process the packet for the data_type int
void print_data_int(packet *new_packet) {

    uint8_t sign = new_packet->content[0];
    uint32_t int_value = ntohl(*(uint32_t *)(new_packet->content + 1));
    
    if (sign == 0 || int_value == 0)
        cout << int_value << "\n";
    else
        cout << "-" << int_value << "\n";
}

// Process the packet for the data_type short
void print_data_short(packet *new_packet) {
    uint16_t short_value = ntohs(*(uint16_t *)(new_packet->content));
    double short_double_value = short_value / 100.0;
    cout << fixed << setprecision(2) << short_double_value << "\n";
}

// Process the packet for the data_type float
void print_data_float(packet *new_packet) {
    uint8_t sign_float = new_packet->content[0];
    uint32_t float_value = ntohl(*(uint32_t *)(new_packet->content + 1));
    uint8_t power = new_packet->content[5];
    double float_double_value = float_value / pow(10, power);
    if (sign_float == 0)
        cout << fixed << setprecision(power) << float_double_value << "\n";
    else
        cout << "-" << fixed << setprecision(power) << float_double_value << "\n";
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    DIE(argc != 4, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>");

    int i, rc, nfds = 0;
    char send_command[MAX_CMD_LEN];
    char buffer[BUFLEN];
    struct pollfd fds[MAX_CONNECTIONS];

    uint16_t port;
    rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Create the tcp socket and add it to the poll
    int tcp_socket = create_socket(port, SOCK_STREAM, true);

    strcpy(buffer, argv[1]);
    send_all(tcp_socket, buffer, sizeof(buffer));

    nfds = add_in_poll(fds, nfds, STDIN_FILENO);
    nfds = add_in_poll(fds, nfds, tcp_socket);
    
    while(true) {
        poll(fds, nfds, -1);
        for (i = 0; i < nfds; i++) {
            // If the event is from the stdin then we have a commnad to proccess
            if (fds[i].revents & POLLIN && fds[i].fd == STDIN_FILENO) {
                cin.getline(buffer, BUFLEN);
                char *token = strtok(buffer, " ");
                char *topic = strtok(NULL, " ");
                char *third_token = strtok(NULL, " ");

                // Check if the command is valid and send the commnad to server
                if (strncmp(token, "exit", 4) == 0){
                    tcp_close_connection(tcp_socket);
                    return 0;

                } else if (third_token != NULL || topic == NULL){
                    cout << "Commands accepted: exit , subscribe <Topic>, unsubscribe <Topic>\n";

                } else if (strlen(topic) > MAX_TOPIC_LEN) {
                    cout << "Topic too long MAX:50\n";

                } else if (strncmp(token, "subscribe", 9) == 0) {
                    strcpy(send_command, "subscribe ");
                    strcat(send_command, topic);
                    rc = send_all(tcp_socket, send_command, sizeof(send_command));
                    cout << "Subscribed to topic " << topic << "\n";
    
                } else if (strncmp(token, "unsubscribe", 11) == 0){
                    strcpy(send_command, "unsubscribe ");
                    strcat(send_command, topic);
                    rc = send_all(tcp_socket, send_command, sizeof(send_command));
                    cout << "Unsubscribed from topic " << topic << "\n";

                } else {
                    cout << "Commands accepted: exit , subscribe <Topic>, unsubscribe <Topic>\n";
                }
            // If the event is from the tcp_socket then we have a new packet to proccess
            } else if (fds[i].revents & POLLIN && fds[i].fd == tcp_socket) {
                packet *new_packet = new packet;
                memset(new_packet, 0, sizeof(packet));

                rc = recv_all(tcp_socket, (char *)&new_packet->head_packet, sizeof(header));
                DIE(rc < 0, "recv");

                // If the rc is 0 then the server has closed the connection
                if (rc == 0) {
                    tcp_close_connection(tcp_socket);
                    return 0;
                }

                // Else we print the packet
                cout << inet_ntoa(*(struct in_addr *)&new_packet->head_packet.ip) << ":" 
                << new_packet->head_packet.port << " - " << new_packet->head_packet.topic 
                << " - " << types[new_packet->head_packet.data_type] << " - ";
                
                new_packet->content = new char[new_packet->head_packet.size];
                recv_all(tcp_socket, new_packet->content, new_packet->head_packet.size);

                // Process the packet based on the data_type and print it
                switch (new_packet->head_packet.data_type) {
                    case 0:
                        print_data_int(new_packet);
                        break;
                    case 1:
                        print_data_short(new_packet);
                        break;
                    case 2:
                        print_data_float(new_packet);
                        break;
                    case 3:
                        cout << new_packet->content << "\n";
                        break;
                }

                delete new_packet->content;
                delete new_packet;
            }
        }
    }

    return 0;
}