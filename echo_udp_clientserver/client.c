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
    const char *serv_hostname = "127.0.0.1";
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    if (inet_pton(AF_INET, serv_hostname, &serv_addr.sin_addr) <= 0) {
         perror("Hostname error");
         exit(EXIT_FAILURE);
    }

    int client_socket;
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    char message[2048] = {0};
    printf("Entre ton message: ");
    if (fgets(message, sizeof(message), stdin) == NULL) {
        perror("message error");
        exit(EXIT_FAILURE);
    }
    size_t len_message = strlen(message);
    if (len_message != 0 && message[len_message - 1] == '\n') {
        message[len_message - 1] = '\0';
    }

    ssize_t sent = sendto(client_socket,
                         message, 
                         strlen(message),
                         0,
                         (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    
    if (sent < 0) {
        perror("transmission error");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char buffer[2048] = {0};
    struct sockaddr_in sender;
    socklen_t senderlen = sizeof(sender);

    ssize_t recvd = recvfrom(client_socket,
                             buffer,
                             sizeof(buffer) - 1,
                             0,
                             (struct sockaddr *) &sender,
                             &senderlen);
    buffer[recvd] = '\0';
    printf("message recu (%zd octets) : %s\n", recvd, buffer);

    close(client_socket);
    return 0;

}
