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
    const char *servername = "127.0.0.1";
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    if (inet_pton(AF_INET, servername, &serv_addr.sin_addr) <= 0) {
        perror("server hostname");
        exit(EXIT_FAILURE);
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, 
        (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("connect error");
            exit(EXIT_FAILURE);
    }
    char message[2048] = {0};
    printf("write something lowercase: ");
    if (fgets(message, sizeof(message), stdin) == NULL) {
        perror("message");
        exit(EXIT_FAILURE);
    }
    ssize_t sent = send(client_socket, message, strlen(message), 0);
    if (sent < 0) {
        perror("sent");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    char buffer[2048] = {0};
    ssize_t recvd = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (recvd < 0) {
        perror("received");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    buffer[recvd] = '\0';
    if (recvd > 0 && buffer[recvd - 1] == '\n') {
        buffer[recvd - 1] = '\0';
    }
    char ip[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &serv_addr.sin_addr, ip, sizeof(ip)) == NULL) {
        perror("IP");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("From server: %zd octets from %s:%d --> %s\n",
                    recvd, ip, ntohs(serv_addr.sin_port), buffer);


    close(client_socket);
    return 0;
}
