#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "common.h"

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t *result = malloc(sizeof(uint64_t));
    *result = Factorial(fargs);
    return (void *)result;
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (1) {
        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        port = atoi(optarg);
                        if (port <= 0) {
                            fprintf(stderr, "Invalid port number: %d\n", port);
                            return 1;
                        }
                        break;
                    case 1:
                        tnum = atoi(optarg);
                        if (tnum <= 0) {
                            fprintf(stderr, "Invalid thread number: %d\n", tnum);
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                printf("Arguments error\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = SocketCreate();
    if (server_fd < 0) return 1;

    SocketSetReusable(server_fd);

    if (SocketBind(server_fd, port) < 0) {
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        PrintError("Could not listen on socket");
        close(server_fd);
        return 1;
    }

    printf("Server successfully started and listening at port %d\n", port);

    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            PrintError("Could not establish new connection");
            continue;
        }

        printf("New connection accepted\n");

        while (1) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;
            char from_client[buffer_size];
            int read_bytes = SocketReceiveData(client_fd, from_client, buffer_size);

            if (read_bytes == 0) {
                printf("Client disconnected\n");
                break;
            }
            if (read_bytes < 0) {
                PrintError("Client read failed");
                break;
            }
            if ((unsigned int)read_bytes < buffer_size) {
                fprintf(stderr, "Client send wrong data format\n");
                break;
            }

            uint64_t begin = 0;
            uint64_t end = 0;
            uint64_t mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

            printf("Received: range %lu-%lu mod %lu\n", begin, end, mod);

            if (begin > end) {
                fprintf(stderr, "Invalid range: begin > end\n");
                break;
            }

            // Распределяем работу между потоками
            pthread_t threads[tnum];
            struct FactorialArgs args[tnum];
            
            uint64_t chunk_size = (end - begin + 1) / tnum;
            uint64_t remainder = (end - begin + 1) % tnum;
            uint64_t current_begin = begin;
            
            for (int i = 0; i < tnum; i++) {
                args[i].begin = current_begin;
                args[i].end = current_begin + chunk_size - 1 + ((uint64_t)i < remainder ? 1 : 0);
                args[i].mod = mod;
                current_begin = args[i].end + 1;

                if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
                    fprintf(stderr, "Error: pthread_create failed!\n");
                    break;
                }
            }

            uint64_t total = 1;
            for (int i = 0; i < tnum; i++) {
                uint64_t *result = NULL;
                pthread_join(threads[i], (void **)&result);
                if (result) {
                    total = MultModulo(total, *result, mod);
                    free(result);
                }
            }

            printf("Computed result: %lu\n", total);

            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            if (SocketSendData(client_fd, buffer, sizeof(total)) < 0) {
                PrintError("Can't send data to client");
                break;
            }
        }

        close(client_fd);
        printf("Connection closed\n");
    }

    close(server_fd);
    return 0;
}