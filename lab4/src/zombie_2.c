#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


void demo_single_zombie() {
    printf("1: Создание одного зомби-процесса\n");
    
    printf("Создаем дочерний процесс...\n");
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Дочерний процесс [PID: %d] запущен и сразу завершается\n", getpid());
        exit(0);
    } else if (pid > 0) {
        printf("Родительский процесс [PID: %d] создал дочерний [PID: %d]\n", getpid(), pid);
        printf("Родитель НЕ вызывает wait() - создается зомби!\n");
        printf("Зомби будет висеть 5 секунд...\n\n");
        
        sleep(5);
        
        printf("Родительский процесс вызывает wait()...\n");
        wait(NULL);
        printf("Зомби-процесс убран!\n");
    } else {
        perror("fork failed");
    }
}

void demo_multiple_zombies() {
    printf("2: Создание нескольких зомби-процессов\n");
    
    int count = 3;
    printf("Создаем %d дочерних процессов...\n", count);
    
    pid_t pids[3];
    
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            printf("Дочерний %d [PID: %d] завершается\n", i+1, getpid());
            exit(0);
        } else if (pid > 0) {
            pids[i] = pid;
            printf("Создан дочерний %d [PID: %d]\n", i+1, pid);
        } else {
            perror("fork failed");
        }
    }
    
    printf("\nВсе дочерние процессы завершены, но родитель НЕ вызывает wait()\n");
    printf("Будут созданы %d зомби-процессов!\n", count);
    printf("Зомби будут висеть 7 секунд...\n\n");
    
    sleep(7);
    
    printf("Убираем всех зомби-процессов с помощью wait()...\n");
    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    printf("Все %d зомби-процессов убраны!\n", count);
}

void demo_proper_cleanup() {
    printf("3: Правильная обработка процессов\n");
    
    printf("Создаем дочерний процесс с правильной обработкой...\n");
    pid_t pid = fork();
    
    if (pid == 0) {
    
        printf("Дочерний процесс [PID: %d] работает...\n", getpid());
        sleep(2);
        printf("Дочерний процесс [PID: %d] завершается с кодом 42\n", getpid());
        exit(42);
    } else if (pid > 0) {
        printf("Родительский процесс ожидает завершения дочернего...\n");
        
        int status;
        pid_t finished_pid = wait(&status);
        
        if (WIFEXITED(status)) {
            printf("Дочерний процесс [PID: %d] завершился с кодом: %d\n", 
                   finished_pid, WEXITSTATUS(status));
        }
        printf("Зомби-процесс НЕ создан благодаря своевременному вызову wait()!\n");
    } else {
        perror("fork failed");
    }
}

void demo_zombie_monitoring() {
    printf("4: Мониторинг зомби-процессов\n");
    
    printf("Создаем временный зомби для демонстрации мониторинга...\n");
    
    pid_t pid = fork();
    if (pid == 0) {
        exit(0); 
    } else if (pid > 0) {
        printf("Создан дочерний процесс [PID: %d]\n", pid);
        printf("Создаем зомби (не вызываем wait)...\n\n");
        
        sleep(3);
        
        printf("Убираем зомби...\n");
        wait(NULL);
    }
}


void wait_for_continue(const char* message) {
    printf("\n%s", message);
    printf("Нажмите Enter для продолжения...");
    getchar(); 
}

int main() {
    printf("   1. Создание одного зомби-процесса\n");
    printf("   2. Создание нескольких зомби-процессов\n");
    printf("   3. Правильную обработку процессов\n");
    printf("   4. Мониторинг зомби-процессов\n\n");
    
    
    demo_single_zombie();
    
    demo_multiple_zombies();
    
    demo_proper_cleanup();
    
    demo_zombie_monitoring();
    
    return 0;
}