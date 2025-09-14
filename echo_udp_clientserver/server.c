#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int serv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serv_socket < 0) {
        perror("socket error:");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serv_socket,
       (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("Bind error:");
            exit(EXIT_FAILURE);
    }
    printf("Server ready to receive\n");

    for (;;) {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t lenclient = sizeof(client_addr);

        char buffer[2048] = {0};
        ssize_t recvd = recvfrom(serv_socket,
                                 buffer, sizeof(buffer) - 1,
                                 0,
                                 (struct sockaddr *) &client_addr, &lenclient);
        if (recvd < 0) {
            perror("received error");
            exit(EXIT_FAILURE);
        } else {
            buffer[recvd] = '\0';
        }

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("Recu %zd octets de %s:%d --> %s\n", recvd, ip,
                                                    ntohs(client_addr.sin_port),
                                                    buffer);
        for (ssize_t i = 0; i < recvd; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] -= 'a' - 'A';
            }
        }

        ssize_t sent = sendto(serv_socket, buffer, recvd, 0,
                              (struct sockaddr *) &client_addr, lenclient);
        if (sent < 0) {
            perror("send error:");
            exit(EXIT_FAILURE);
        }

    }

    close(serv_socket);
    return 0;
}
