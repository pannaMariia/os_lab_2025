CC=gcc
CFLAGS=-I.
TARGETS=sequential_min_max parallel_min_max runner

# Основная цель - собирает все программы
all: $(TARGETS)

# Последовательная версия
sequential_min_max: sequential_min_max.o utils.o find_min_max.o
	$(CC) -o $@ $^ $(CFLAGS)

# Параллельная версия  
parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) -o $@ $^ $(CFLAGS)

# Программа для запуска sequential_min_max
runner: runner.o
	$(CC) -o $@ $^ $(CFLAGS)

# Объектные файлы
sequential_min_max.o: sequential_min_max.c utils.h find_min_max.h
	$(CC) -o $@ -c $< $(CFLAGS)

parallel_min_max.o: parallel_min_max.c utils.h find_min_max.h
	$(CC) -o $@ -c $< $(CFLAGS)

runner.o: runner.c
	$(CC) -o $@ -c $< $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -o $@ -c $< $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -o $@ -c $< $(CFLAGS)

# Очистка
clean:
	rm -f *.o $(TARGETS)

# Пересборка
rebuild: clean all

# Отладочная версия
debug: CFLAGS += -g
debug: rebuild

# Псевдо-цели
.PHONY: all clean rebuild debug