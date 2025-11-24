CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -pthread
TARGETS = client server
LIBRARY = libcommon.a
SOURCES = client.c server.c common.c
OBJECTS = $(SOURCES:.c=.o)
LIB_OBJECTS = common.o

all: $(TARGETS)

# Статическая библиотека
$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $@ $^

# Клиент с использованием библиотеки
client: client.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ client.o -L. -lcommon

# Сервер с использованием библиотеки  
server: server.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ server.o -L. -lcommon

# Объектные файлы
%.o: %.c common.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Очистка
clean:
	rm -f $(TARGETS) $(OBJECTS) $(LIBRARY) *.gch

# Установка
install: all
	sudo cp client server /usr/local/bin/

# Запуск тестового сервера
run-server: server
	./server --port 20001 --tnum 4

# Запуск тестового клиента
run-client: client
	./client --k 100 --mod 1000000 --servers servers.txt

# Сборка с отладочной информацией
debug: CFLAGS += -g -O0
debug: clean all

# Показать размеры файлов
size: all
	@echo "Размеры исполняемых файлов:"
	@size client server
	@echo "Размер библиотеки:"
	@size $(LIBRARY)



.PHONY: all clean install run-server run-client debug size help