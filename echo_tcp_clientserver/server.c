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
    int serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serv_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind");
        close(serv_socket);
        exit(EXIT_FAILURE);
    }
    if (listen(serv_socket, 1) < 0) {
        perror("Listen");
        close(serv_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is ready.\n");
    for (;;) {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t clientlen = sizeof(client_addr);

        int client_socket = accept(serv_socket,
                                   (struct sockaddr *) &client_addr,
                                    &clientlen);
        if (client_socket < 0) {
            perror("Accept");
            continue;
        }
        char buffer[2048] = {0};
        ssize_t recvd = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recvd < 0) {
            perror("Receive");
            close(client_socket);
            continue;
        }
        if (recvd == 0) {
            close(client_socket);
            continue;
        }
        buffer[recvd] = '\0';
        if (recvd > 0 && buffer[recvd - 1] == '\n') {
                buffer[recvd - 1] = '\0';
                recvd--;
        }
        for (ssize_t i = 0; i < recvd; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] -= 'a' - 'A';
            }
        }
        ssize_t sent = send(client_socket, buffer, recvd, 0);
        if (sent < 0) {
            perror("Sent");
            close(client_socket);
            continue;
        }
        printf("Sent\n");
        close(client_socket);
    }
    close(serv_socket);
    return 0;
}
