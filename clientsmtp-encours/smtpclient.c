#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

static ssize_t recvd(int fd, char *buff, size_t len);
static ssize_t sendall(int fd,const char *buff, size_t len);
static ssize_t sendf(int fd,const char *buff, const char *arg);

int main(void)
{
    const char *server_name = "127.0.1.1";
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12000);
    if (inet_pton(AF_INET, server_name, &servaddr.sin_addr) < 1) {
        perror("inet_pton address");
        exit(EXIT_FAILURE);
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (connect(client_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("conected to server.\n");
    char buffer[2048] = {0};
    ssize_t n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);

    done:
        if (n < 0) {
            fprintf(stderr, "error: %d: %s\n", errno, strerror(errno));
        } else if (n == 0) {
            printf("conection closed by server\n");
        }
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        return 0;
}

static ssize_t recvd(int fd, char *buff, size_t len) 
{
    if (len == 0) {
        errno = EINVAL; 
        return -1;
    }

    ssize_t n;
    do {
        n = recv(fd, buff, len - 1, 0);
    } while (n < 0 && errno == EINTR);
    
    if (n > 0) {
        buff[n] = '\0';
    }

    return n;
}
static ssize_t sendf(int fd,const char *fmt, const char *arg) 
{
    if (!fmt) {
        errno = EINVAL;
        return -1;
    }
    
    char buffer[1024];
    int n = arg
        ? snprintf(buffer, sizeof(buffer), fmt, arg)
        : snprintf(buffer, sizeof(buffer), "%s", fmt);

    if (n < 0) {
        errno = EINVAL;
        return -1;
    }
    if ((size_t)n >= sizeof(buffer)) {
        errno = EMSGSIZE;
        return -1;
    }

    return sendall(fd, buffer, (size_t) n);
}
static ssize_t sendall(int fd,const char *buf, size_t len) 
{
    const char *p = buf;
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(fd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            errno = EPIPE;
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
    
}
