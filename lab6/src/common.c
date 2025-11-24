#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }

    if (errno != 0)
        return false;

    *val = i;
    return true;
}

int SocketCreate() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        PrintError("Can not create socket");
    }
    return sockfd;
}

void SocketSetReusable(int sockfd) {
    int opt_val = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0) {
        PrintError("Setsockopt failed");
    }
}

int SocketBind(int sockfd, int port) {
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int result = bind(sockfd, (struct sockaddr *)&server, sizeof(server));
    if (result < 0) {
        fprintf(stderr, "Can not bind to socket on port %d: %s\n", port, strerror(errno));
    }
    return result;
}

int SocketConnect(int sockfd, const char* ip, int port) {
    struct hostent *hostname = gethostbyname(ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", ip);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (hostname->h_addr_list[0] != NULL) {
        memcpy(&server_addr.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
    } else {
        fprintf(stderr, "No address found for %s\n", ip);
        return -1;
    }

    return connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int SocketSendData(int sockfd, const void* data, size_t size) {
    ssize_t result = send(sockfd, data, size, 0);
    return (int)result;
}

int SocketReceiveData(int sockfd, void* buffer, size_t size) {
    ssize_t result = recv(sockfd, buffer, size, 0);
    return (int)result;
}

void PrintError(const char* message) {
    fprintf(stderr, "%s: %s\n", message, strerror(errno));
}