#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = 0;

  while (true) {
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  int *pipefd = NULL;
  char **filenames = NULL;
  
  if (!with_files) {
    pipefd = malloc(2 * pnum * sizeof(int));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd + 2*i) == -1) {
        perror("pipe");
        return 1;
      }
    }
  } else {
    filenames = malloc(pnum * sizeof(char*));
    for (int i = 0; i < pnum; i++) {
      filenames[i] = malloc(20 * sizeof(char));
      sprintf(filenames[i], "min_max_%d.txt", i);
    }
  }

  int active_child_processes = 0;
  pid_t *child_pids = malloc(pnum * sizeof(pid_t));

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid;
      if (child_pid == 0) {
        int chunk_size = array_size / pnum;
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, begin, end);
        
        if (with_files) {
          FILE *file = fopen(filenames[i], "w");
          if (file == NULL) {
            perror("fopen");
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          close(pipefd[2*i]);
          write(pipefd[2*i + 1], &local_min_max.min, sizeof(int));
          write(pipefd[2*i + 1], &local_min_max.max, sizeof(int));
          close(pipefd[2*i + 1]);
        }
        free(array);
        if (!with_files) free(pipefd);
        if (with_files) {
          for (int j = 0; j < pnum; j++) free(filenames[j]);
          free(filenames);
        }
        free(child_pids);
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      free(child_pids);
      return 1;
    }
  }

  if (timeout > 0) {
    pid_t timeout_pid = fork();
    if (timeout_pid == 0) {
      sleep(timeout);
      exit(0);
    } else if (timeout_pid > 0) {
      int timeout_status;
      pid_t finished_pid = waitpid(timeout_pid, &timeout_status, WNOHANG);
      
      if (finished_pid == 0) {
        pid_t completed_pid;
        int completed_count = 0;
        
        while (completed_count < pnum && (completed_pid = waitpid(-1, NULL, WNOHANG)) != timeout_pid) {
          if (completed_pid > 0) {
            completed_count++;
            active_child_processes--;
            
            for (int i = 0; i < pnum; i++) {
              if (child_pids[i] == completed_pid) {
                child_pids[i] = -1;
                break;
              }
            }
          } else if (completed_pid == 0) {
            if (waitpid(timeout_pid, &timeout_status, WNOHANG) == timeout_pid) {
              break;
            }
            sleep(0); // Замена usleep
          }
        }
        
        if (active_child_processes > 0) {
          printf("Timeout expired (%d seconds), killing child processes...\n", timeout);
          for (int i = 0; i < pnum; i++) {
            if (child_pids[i] > 0) {
              kill(child_pids[i], SIGKILL);
              waitpid(child_pids[i], NULL, 0);
              active_child_processes--;
            }
          }
        }
        
        if (waitpid(timeout_pid, NULL, WNOHANG) == 0) {
          kill(timeout_pid, SIGKILL);
          waitpid(timeout_pid, NULL, 0);
        }
      } else {
        waitpid(timeout_pid, NULL, 0);
      }
    }
  } else {
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (child_pids[i] == -1) continue;

    if (with_files) {
      FILE *file = fopen(filenames[i], "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filenames[i]);
      }
    } else {
      close(pipefd[2*i + 1]);
      if (read(pipefd[2*i], &min, sizeof(int)) > 0 &&
          read(pipefd[2*i], &max, sizeof(int)) > 0) {
      }
      close(pipefd[2*i]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (!with_files) free(pipefd);
  if (with_files) {
    for (int i = 0; i < pnum; i++) free(filenames[i]);
    free(filenames);
  }
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}