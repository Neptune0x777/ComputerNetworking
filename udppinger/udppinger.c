#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>



int main(void)
{
    int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_client < 0) {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    struct timeval tv = {1, 0};
    if (setsockopt(socket_client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(socket_client);
        exit(EXIT_FAILURE);
    }

    const char *addr = "127.0.1.1";
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(12000);
    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        perror("Inet pton");
        close(socket_client);
        exit(EXIT_FAILURE);
    } 
    
    char sendbuffer[256] = {0};
    char recvbuffer[256] = {0};

    for (int seq = 1; seq <= 10; seq++) {
        struct timespec time_1, time_2;
        clock_gettime(CLOCK_MONOTONIC, &time_1);

        int slen = snprintf(sendbuffer, sizeof(sendbuffer), "Ping %d", seq);
        if (slen <= 0) {
            perror("snprintf");
            break;
        }

        if (sendto(socket_client, sendbuffer, slen, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            printf("Send error ping %d\n", seq);
            continue;
        }
        struct sockaddr_in from_addr;
        memset(&from_addr, 0, sizeof(from_addr));
        socklen_t lenaddr = sizeof(from_addr);

        ssize_t recvd = recvfrom(socket_client, recvbuffer, sizeof(recvbuffer) - 1, 0, (struct sockaddr *) &from_addr, &lenaddr);
        if (recvd < 0) {
            printf("Timeout or recpt error ping %d\n", seq);
            continue;
        }
        recvbuffer[recvd] = '\0';
        clock_gettime(CLOCK_MONOTONIC, &time_2);

        time_t dsec = time_2.tv_sec - time_1.tv_sec;
        long   dns  = time_2.tv_nsec - time_1.tv_nsec;
        if (dns < 0) { dns += 1000000000L; dsec -= 1; }
        double rtt_ms = (double)dsec * 1000.0 + (double)dns / 1e6;

        printf("Ping %d: RTT = %.3f ms | Réponse: \"%s\"\n", seq, rtt_ms, recvbuffer);
    }

    close(socket_client);
    return 0;
}