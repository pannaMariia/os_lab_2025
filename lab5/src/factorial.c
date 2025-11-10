#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Структура для передачи данных в поток
typedef struct {
    int start;
    int end;
    long long mod;
    long long partial_result;
    long long *global_result;
    pthread_mutex_t *mutex;
} thread_data_t;

// Функция для потока - вычисляет частичное произведение
void* calculate_partial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->partial_result = 1;
    
    for (int i = data->start; i <= data->end; i++) {
        data->partial_result = (data->partial_result * i) % data->mod;
    }
    
    // Защищаем доступ к глобальному результату мьютексом
    pthread_mutex_lock(data->mutex);
    *data->global_result = (*data->global_result * data->partial_result) % data->mod;
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

// Функция для парсинга аргументов командной строки
int parse_arguments(int argc, char* argv[], int* k, int* pnum, long long* mod) {
    *k = 0;
    *pnum = 1;
    *mod = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            *k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            *pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            *mod = atoll(argv[i] + 6);
        }
    }
    
    if (*k <= 0 || *pnum <= 0 || *mod <= 0) {
        fprintf(stderr, "Error: All parameters must be positive integers\n");
        fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        fprintf(stderr, "Example: %s -k 10 --pnum=4 --mod=1000000\n", argv[0]);
        return -1;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    int k, pnum;
    long long mod;
    long long global_result = 1;
    
    // Парсинг аргументов командной строки
    if (parse_arguments(argc, argv, &k, &pnum, &mod) != 0) {
        return 1;
    }
    
    printf("Calculating %d! mod %lld using %d threads\n", k, mod, pnum);
    
    // Если количество потоков больше k, ограничиваем его
    if (pnum > k) {
        pnum = k;
        printf("Adjusted number of threads to %d (cannot exceed k)\n", pnum);
    }
    
    // Инициализация мьютекса
    pthread_mutex_t mutex;
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Error: mutex init failed\n");
        return 1;
    }
    
    // Создание потоков
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    // Создание и запуск потоков
    for (int i = 0; i < pnum; i++) {
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_per_thread - 1;
        
        // Распределяем остаток по первым потокам
        if (i < remainder) {
            thread_data[i].end++;
        }
        
        thread_data[i].mod = mod;
        thread_data[i].global_result = &global_result;
        thread_data[i].mutex = &mutex;
        
        current_start = thread_data[i].end + 1;
        
        if (pthread_create(&threads[i], NULL, calculate_partial, &thread_data[i]) != 0) {
            fprintf(stderr, "Error: thread creation failed\n");
            return 1;
        }
        
        printf("Thread %d: calculating from %d to %d\n", 
               i, thread_data[i].start, thread_data[i].end);
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Error: thread join failed\n");
            return 1;
        }
    }
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);
    
    // Вывод результата
    printf("\nResult: %d! mod %lld = %lld\n", k, mod, global_result);
    
    return 0;
}