#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server < 0) {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind");
        close(socket_server);
        exit(EXIT_FAILURE);
    }
    if (listen(socket_server, 1) < 0) {
        perror("Listen");
        close(socket_server);
        exit(EXIT_FAILURE);
    }
    printf("Server ready!\n");

    for (;;) {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t clientlen = sizeof(client_addr);

        int client_socket = accept(socket_server, (struct sockaddr *) &client_addr, &clientlen);
        if (client_socket < 0) {
            perror("Accept");
            continue;
        }
        char buffer[2048] = {0};
        ssize_t recvd = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recvd < 0) {
            perror("recv");
            close(client_socket);
            continue;
        }
        if (recvd == 0) {
            close(client_socket);
            continue;
        }
        buffer[recvd] = '\0';
        char method[16], path[256], version[16];
        if (sscanf(buffer, "%15s %255s %15s", method, path, version) != 3) {
            perror("sscanf");
            close(client_socket);
            continue;
        }
        if (strcmp(method, "GET") != 0) {
            const char *response = 
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Allow: GET\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(client_socket, response, strlen(response), 0);
            close(client_socket);
            continue;
        }
        char *filename = path[0] == '/' ? path + 1 : path;
        if (*filename == '\0') filename = "index.html";
        if (strstr(filename, "..")) {
            const char *response = 
                "HTTP/1.1 403 Forbidden\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(client_socket, response, strlen(response), 0);
            close(client_socket);
            continue;
        }
        FILE *fp = fopen(filename, "rb");
        if (fp) {
            const char *header = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n";
            send(client_socket, header, strlen(header), 0);
            char buffer_file[1024] = {0};
            size_t n;

            while ((n = fread(buffer_file, 1, sizeof(buffer_file), fp)) > 0) {
                send(client_socket, buffer_file, n, 0);
            }
            fclose(fp);
        } else {
            const char *not_found = 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html><body><h1>404 Not Found</h1></body></html>";
            send(client_socket, not_found, strlen(not_found), 0);
        }
        close(client_socket);

    }
    close(socket_server);
    return 0;
}
