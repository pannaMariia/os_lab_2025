#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>

#include "common.h"  


struct Server {
    char ip[255];      // IP-адрес сервера (строка до 255 символов)
    int port;          // Порт сервера
};

// Структура для задачи, отправляемой на сервер
struct ServerTask {
    struct Server server;  // Информация о сервере
    uint64_t begin;        // Начало диапазона вычислений
    uint64_t end;          // Конец диапазона вычислений  
    uint64_t mod;          // Модуль для вычислений
    uint64_t result;       // Результат, полученный от сервера
    int success;           // Флаг успешного выполнения (1 - успех, 0 - ошибка)
};


bool ReadServerConfig(const char *filename, struct Server **servers, int *servers_num) {
    // Открываем файл для чтения
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open servers file: %s\n", filename);
        return false;
    }

    // Первый проход: подсчитываем количество серверов в файле
    *servers_num = 0;
    char line[255];
    while (fgets(line, sizeof(line), file)) {
        (*servers_num)++;  // Увеличиваем счетчик для каждой строки
    }

    // Проверяем, что в файле есть серверы
    if (*servers_num == 0) {
        fprintf(stderr, "No servers found in file\n");
        fclose(file);
        return false;
    }

    // Выделяем память под массив серверов
    *servers = malloc(sizeof(struct Server) * (*servers_num));
    if (!*servers) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return false;
    }

    // Второй проход: читаем данные серверов
    rewind(file);  // Возвращаемся в начало файла
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < *servers_num) {
        // Убираем символ новой строки из конца строки
        line[strcspn(line, "\n")] = 0;
        
        // Ищем разделитель ':' между IP и портом
        char *colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid server format: %s. Expected ip:port\n", line);
            continue;  // Пропускаем некорректные строки
        }
        
        // Разделяем строку на IP и порт
        *colon = '\0';  // Заменяем ':' на нулевой символ, разделяя строку
        
        // Копируем IP-адрес (с защитой от переполнения буфера)
        strncpy((*servers)[index].ip, line, sizeof((*servers)[index].ip) - 1);
        (*servers)[index].ip[sizeof((*servers)[index].ip) - 1] = '\0';  // Гарантируем завершающий ноль
        
        // Преобразуем порт из строки в число
        (*servers)[index].port = atoi(colon + 1);
        if ((*servers)[index].port <= 0) {
            fprintf(stderr, "Invalid port number: %s\n", colon + 1);
            continue;  // Пропускаем серверы с некорректным портом
        }
        
        index++;  // Переходим к следующему серверу
    }

    fclose(file);
    *servers_num = index;  // Обновляем фактическое количество прочитанных серверов
    return true;
}

//Функция, выполняемая в отдельном потоке для взаимодействия с одним сервером
void *ProcessServerTask(void *args) {
    struct ServerTask *task = (struct ServerTask *)args;

    // Создаем сокет для соединения с сервером
    int sockfd = SocketCreate();
    if (sockfd < 0) {
        task->success = 0; 
        return NULL;
    }

    // Устанавливаем соединение с сервером
    if (SocketConnect(sockfd, task->server.ip, task->server.port) < 0) {
        close(sockfd);     
        task->success = 0;  
        return NULL;
    }

    // Выводим информацию о отправляемой задаче
    printf("Sending to server %s:%d: range %lu-%lu mod %lu\n", 
           task->server.ip, task->server.port, task->begin, task->end, task->mod);

    // Подготавливаем данные для отправки: три числа uint64_t в бинарном формате
    char task_data[sizeof(uint64_t) * 3];  // Буфер для begin, end, mod
    memcpy(task_data, &task->begin, sizeof(uint64_t));                    // Копируем начало диапазона
    memcpy(task_data + sizeof(uint64_t), &task->end, sizeof(uint64_t));   // Копируем конец диапазона
    memcpy(task_data + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t)); // Копируем модуль

    // Отправляем задание серверу
    if (SocketSendData(sockfd, task_data, sizeof(task_data)) < 0) {
        close(sockfd);
        task->success = 0;
        return NULL;
    }

    // Получаем результат от сервера
    char response[sizeof(uint64_t)];  
    int recv_result = SocketReceiveData(sockfd, response, sizeof(response));
    if (recv_result < (int)sizeof(uint64_t)) {
        close(sockfd);
        task->success = 0;
        return NULL;
    }

    // Копируем полученный результат в структуру задачи
    memcpy(&task->result, response, sizeof(uint64_t));
    printf("Received from server %s:%d: %lu\n", 
           task->server.ip, task->server.port, task->result);

    task->success = 1; 
    close(sockfd);      
    return NULL;
}


int main(int argc, char **argv) {
    // Переменные для хранения аргументов командной строки
    uint64_t k = 0;           
    uint64_t mod = 0;   
    char servers_file[255] = {'\0'}; 
    int k_set = 0, mod_set = 0, servers_set = 0; 

    // Обработка аргументов командной строки
    while (1) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},        // --k <число>
            {"mod", required_argument, 0, 0},      // --mod <модуль>
            {"servers", required_argument, 0, 0},  // --servers <файл>
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
                        // Копируем путь к файлу серверов с защитой от переполнения
                        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                        servers_file[sizeof(servers_file) - 1] = '\0';
                        servers_set = 1;  // Отмечаем, что servers установлен
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

    // Читаем конфигурацию серверов из файла
    struct Server *servers = NULL;
    int servers_num = 0;
    if (!ReadServerConfig(servers_file, &servers, &servers_num)) {
        return 1;
    }

    // Проверяем, что найдены валидные серверы
    if (servers_num == 0) {
        fprintf(stderr, "No valid servers found\n");
        free(servers);
        return 1;
    }

    printf("Found %d servers, k = %lu, mod = %lu\n", servers_num, k, mod);

    // Выделяем память под задачи и потоки
    struct ServerTask *tasks = malloc(sizeof(struct ServerTask) * servers_num);
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);

    // Проверяем успешность выделения памяти
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

    // Инициализируем задачи для каждого сервера
    for (int i = 0; i < servers_num; i++) {
        tasks[i].server = servers[i];              
        tasks[i].begin = i * chunk_size + 1;     
        tasks[i].end = (i + 1) * chunk_size;            
        tasks[i].mod = mod;                            
        tasks[i].result = 1;                        
        tasks[i].success = 0;                         

        if (i == servers_num - 1) {
            tasks[i].end += remainder;
        }
    }

    // Запускаем потоки для параллельного взаимодействия с серверами
    printf("Starting parallel communication with %d servers...\n", servers_num);
    
    for (int i = 0; i < servers_num; i++) {
        // Создаем поток для обработки задачи сервера
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

    // Объединяем результаты от всех успешно выполненных задач
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

    free(servers);
    free(tasks);
    free(threads);

    return 0;
}