#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;

int thread1_completed = 0;
int thread2_completed = 0;

void* thread1_function(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    sleep(1);
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 1: Locked mutex2\n");
    
    printf("Thread 1: Entering critical section\n");
    sleep(1);
    printf("Thread 1: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    pthread_mutex_lock(&flag_mutex);
    thread1_completed = 1;
    pthread_mutex_unlock(&flag_mutex);
    
    return NULL;
}

void* thread2_function(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    sleep(1);
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2: Locked mutex1\n");
    
    printf("Thread 2: Entering critical section\n");
    sleep(1);
    printf("Thread 2: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    pthread_mutex_lock(&flag_mutex);
    thread2_completed = 1;
    pthread_mutex_unlock(&flag_mutex);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Deadlock Demonstration ===\n");
    printf("This program will demonstrate a classic deadlock scenario.\n\n");
    
    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);
    
    printf("Waiting for 5 seconds to see if threads complete...\n");
    sleep(5);
    
    // Проверяем статус потоков
    pthread_mutex_lock(&flag_mutex);
    int t1_done = thread1_completed;
    int t2_done = thread2_completed;
    pthread_mutex_unlock(&flag_mutex);
    
    if (!t1_done || !t2_done) {
        printf("\n=== DEADLOCK DETECTED ===\n");
        printf("Thread 1: %s\n", t1_done ? "completed" : "BLOCKED");
        printf("Thread 2: %s\n", t2_done ? "completed" : "BLOCKED");
        printf("\nBoth threads are waiting for resources held by each other!\n");
        printf("Forcing program termination...\n");
        exit(0);
    }
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Program completed successfully\n");
    return 0;
}