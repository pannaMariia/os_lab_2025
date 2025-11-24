#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>

#include "common.h"

struct Server {
    char ip[255];
    int port;
};

struct ServerTask {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    int success;
};

bool ReadServerConfig(const char *filename, struct Server **servers, int *servers_num) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open servers file: %s\n", filename);
        return false;
    }

    *servers_num = 0;
    char line[255];
    while (fgets(line, sizeof(line), file)) {
        (*servers_num)++;
    }

    if (*servers_num == 0) {
        fprintf(stderr, "No servers found in file\n");
        fclose(file);
        return false;
    }

    *servers = malloc(sizeof(struct Server) * (*servers_num));
    if (!*servers) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return false;
    }

    rewind(file);
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < *servers_num) {
        line[strcspn(line, "\n")] = 0;
        
        char *colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid server format: %s. Expected ip:port\n", line);
            continue;
        }
        
        *colon = '\0';
        strncpy((*servers)[index].ip, line, sizeof((*servers)[index].ip) - 1);
        (*servers)[index].ip[sizeof((*servers)[index].ip) - 1] = '\0';
        
        (*servers)[index].port = atoi(colon + 1);
        if ((*servers)[index].port <= 0) {
            fprintf(stderr, "Invalid port number: %s\n", colon + 1);
            continue;
        }
        
        index++;
    }

    fclose(file);
    *servers_num = index;
    return true;
}

void *ProcessServerTask(void *args) {
    struct ServerTask *task = (struct ServerTask *)args;

    int sockfd = SocketCreate();
    if (sockfd < 0) {
        task->success = 0;
        return NULL;
    }

    if (SocketConnect(sockfd, task->server.ip, task->server.port) < 0) {
        close(sockfd);
        task->success = 0;
        return NULL;
    }

    printf("Sending to server %s:%d: range %lu-%lu mod %lu\n", 
           task->server.ip, task->server.port, task->begin, task->end, task->mod);

    // Подготавливаем и отправляем задание
    char task_data[sizeof(uint64_t) * 3];
    memcpy(task_data, &task->begin, sizeof(uint64_t));
    memcpy(task_data + sizeof(uint64_t), &task->end, sizeof(uint64_t));
    memcpy(task_data + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t));

    if (SocketSendData(sockfd, task_data, sizeof(task_data)) < 0) {
        close(sockfd);
        task->success = 0;
        return NULL;
    }

    // Получаем результат
    char response[sizeof(uint64_t)];
    int recv_result = SocketReceiveData(sockfd, response, sizeof(response));
    if (recv_result < (int)sizeof(uint64_t)) {
        close(sockfd);
        task->success = 0;
        return NULL;
    }

    memcpy(&task->result, response, sizeof(uint64_t));
    printf("Received from server %s:%d: %lu\n", 
           task->server.ip, task->server.port, task->result);

    task->success = 1;
    close(sockfd);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};
    int k_set = 0, mod_set = 0, servers_set = 0;

    while (1) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        if (!ConvertStringToUI64(optarg, &k)) {
                            fprintf(stderr, "Invalid k value: %s\n", optarg);
                            return 1;
                        }
                        k_set = 1;
                        break;
                    case 1:
                        if (!ConvertStringToUI64(optarg, &mod)) {
                            fprintf(stderr, "Invalid mod value: %s\n", optarg);
                            return 1;
                        }
                        mod_set = 1;
                        break;
                    case 2:
                        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                        servers_file[sizeof(servers_file) - 1] = '\0';
                        servers_set = 1;
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

    if (!k_set || !mod_set || !servers_set) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Читаем конфигурацию серверов
    struct Server *servers = NULL;
    int servers_num = 0;
    if (!ReadServerConfig(servers_file, &servers, &servers_num)) {
        return 1;
    }

    if (servers_num == 0) {
        fprintf(stderr, "No valid servers found\n");
        free(servers);
        return 1;
    }

    printf("Found %d servers, k = %lu, mod = %lu\n", servers_num, k, mod);

    // Создаем задачи для каждого сервера
    struct ServerTask *tasks = malloc(sizeof(struct ServerTask) * servers_num);
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);

    if (!tasks || !threads) {
        fprintf(stderr, "Memory allocation failed\n");
        free(servers);
        if (tasks) free(tasks);
        if (threads) free(threads);
        return 1;
    }

    // Распределяем работу между серверами
    uint64_t chunk_size = k / servers_num;
    uint64_t remainder = k % servers_num;

    for (int i = 0; i < servers_num; i++) {
        tasks[i].server = servers[i];
        tasks[i].begin = i * chunk_size + 1;
        tasks[i].end = (i + 1) * chunk_size;
        tasks[i].mod = mod;
        tasks[i].result = 1;
        tasks[i].success = 0;

        // Распределяем остаток по серверам
        if (i == servers_num - 1) {
            tasks[i].end += remainder;
        }
    }

    // Запускаем потоки для параллельного взаимодействия с серверами
    printf("Starting parallel communication with %d servers...\n", servers_num);
    
    for (int i = 0; i < servers_num; i++) {
        if (pthread_create(&threads[i], NULL, ProcessServerTask, &tasks[i])) {
            fprintf(stderr, "Error creating thread for server %s:%d\n", 
                    servers[i].ip, servers[i].port);
            tasks[i].success = 0;
        }
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
    }

    // Объединяем результаты
    uint64_t total_result = 1;
    int successful_servers = 0;

    for (int i = 0; i < servers_num; i++) {
        if (tasks[i].success) {
            total_result = MultModulo(total_result, tasks[i].result, mod);
            successful_servers++;
            printf("Server %s:%d completed successfully: %lu\n", 
                   tasks[i].server.ip, tasks[i].server.port, tasks[i].result);
        } else {
            printf("Server %s:%d failed\n", tasks[i].server.ip, tasks[i].server.port);
        }
    }

    if (successful_servers == 0) {
        fprintf(stderr, "All servers failed!\n");
    } else if (successful_servers < servers_num) {
        printf("Warning: only %d out of %d servers completed successfully\n", 
               successful_servers, servers_num);
    }

    printf("\nFinal result: %lu! mod %lu = %lu\n", k, mod, total_result);
    printf("Computed using %d servers in parallel\n", successful_servers);

    // Освобождаем ресурсы
    free(servers);
    free(tasks);
    free(threads);

    return 0;
}