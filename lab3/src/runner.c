#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        printf("Example: %s 42 1000\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        // Дочерний процесс - передаем аргументы БЕЗ --prefix
        printf("Starting sequential_min_max with seed=%s, array_size=%s\n", 
               argv[1], argv[2]);
        
        // Просто передаем аргументы как есть, без --flags
        execl("./sequential_min_max", "sequential_min_max", 
              argv[1],  // seed
              argv[2],  // array_size
              NULL);
        
        perror("exec failed");
        exit(1);
    }
    else {
        // Родительский процесс
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("sequential_min_max completed with exit code: %d\n", 
                   WEXITSTATUS(status));
        }
    }

    return 0;
}