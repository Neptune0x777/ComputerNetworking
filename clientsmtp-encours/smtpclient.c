#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static ssize_t recvd(int fd, char *buff, size_t len);
static ssize_t sendall(int fd,const char *buff, size_t len);
static ssize_t sendf(int fd,const char *buff, const char *arg);

int main(void)
{
    int status = EXIT_SUCCESS;
    const char *server_name = "127.0.0.1";
    const char *host = "client.local";
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(2525);
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
    // Banniere
    char buffer[2048] = {0};
    ssize_t n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    int code;
    sscanf(buffer, "%3d", &code);
    if (code != 220) {
        fprintf(stderr, "smtp server not ready\n");
        status = EXIT_FAILURE;
        goto done;
    }
    printf("220 OK\n");
    // ehlo and protocol
    sendf(client_socket, "EHLO %s\r\n", host);
    for (;;) {
        n = recvd(client_socket, buffer, sizeof(buffer));
        if (n <= 0) goto done;
        printf("received: %s\n", buffer);

        if (sscanf(buffer, "%3d", &code) == 1) {
            if (code != 250) {
                fprintf(stderr, "unexpected code %d\n", code);
                status = EXIT_FAILURE;
                goto done;
            }
            if (strstr(buffer, "250 ")) {
                break;
            } 
        } else {
            if (strstr(buffer, "\r\n250-")) continue;
            if (strstr(buffer, "250 ")) break;
            fprintf(stderr, "server sent invalid EHLO response\n");
            status = EXIT_FAILURE;
            goto done;
        }
    }
    // mailfrom
    sendf(client_socket, "MAIL FROM:<from@test.com>\r\n", NULL);
    n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    sscanf(buffer, "%3d", &code);
    if (code != 250) {
        fprintf(stderr, "mailfrom invalid code: %d\n", code);
        status = EXIT_FAILURE;
        goto done;
    }

    // rcpt to
    sendf(client_socket, "RCPT TO:<rcpt@test.com>\r\n", NULL);
    n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    sscanf(buffer, "%3d", &code);
    if (code != 250) {
        fprintf(stderr, "rcpt to invalid code: %d\n", code);
        status = EXIT_FAILURE;
        goto done;
    }

    // data
    sendf(client_socket, "DATA\r\n", NULL);
    n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    sscanf(buffer, "%3d", &code);
    if (code != 354) {
        fprintf(stderr, "DATA invalid code response: %d\n", code);
        status = EXIT_FAILURE;
        goto done;
    }
    // all data
    const char *data = 
        "From: from@test.com\r\n"
        "To: rcpt@test.com\r\n"
        "Subject: Test\r\n"
        "\r\n"
        "Bonjour,\r\n"
        "Test.\r\n"
        "\r\n.\r\n";
    sendf(client_socket, "%s", data);
    n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    sscanf(buffer, "%3d", &code);
    if (code != 250) {
        fprintf(stderr, "DATA invalid code response: %d\n", code);
        status = EXIT_FAILURE;
        goto done;
    }

    // quit
    sendf(client_socket, "QUIT\r\n", NULL);
    n = recvd(client_socket, buffer, sizeof(buffer));
    if (n <= 0) goto done;
    printf("received: %s\n", buffer);
    sscanf(buffer, "%3d", &code);
    if (code != 221) {
        status = EXIT_FAILURE;
        goto done;
    }
    
    done:
        if (n < 0) {
            fprintf(stderr, "error: %d: %s\n", errno, strerror(errno));
        } else if (n == 0) {
            printf("conection closed by server\n");
        }
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        return status;
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
