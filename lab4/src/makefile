CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200112L
PTHREAD_FLAGS = -pthread

all: parallel_min_max process_memory parallel_sum
#  parallel_min_max
parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	$(CC) $(CFLAGS) -o $@ $^
parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
	$(CC) $(CFLAGS) -c $<
find_min_max.o: find_min_max.c find_min_max.h
	$(CC) $(CFLAGS) -c $<
utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $<
# process_memory
process_memory: process_memory.o
	$(CC) $(CFLAGS) -o $@ $<

process_memory.o: process_memory.c
	$(CC) $(CFLAGS) -c $<
# parallel_sum
parallel_sum: parallel_sum.o sum.o utils.o
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o $@ $^
parallel_sum.o: parallel_sum.c sum.h utils.h
	$(CC) $(CFLAGS) -c $<
sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f *.o parallel_min_max process_memory parallel_sum
.PHONY: all clean