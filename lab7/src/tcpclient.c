#include <arpa/inet.h>   
#include <netinet/in.h> 
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>   
#include <sys/socket.h>  
#include <sys/types.h>  
#include <unistd.h>      

// Макросы для удобства работы с сокетами
#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {

  if (argc < 4) {
    printf("usage: %s <IPaddress> <port> <bufsize>\n", argv[0]);
    exit(1);
  }


  int bufsize = atoi(argv[3]);  
  char buf[bufsize];             // Буфер для чтения и отправки данных
  
  int fd;                        // Дескриптор сокета
  int nread;                     // Количество прочитанных байт
  struct sockaddr_in servaddr;   // Структура для адреса сервера
  
  // Создание TCP сокета 
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating"); 
    exit(1);
  }

  // Инициализация структуры адреса сервера нулями
  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;   

  // Преобразование IP-адреса из строкового представления в двоичное
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("bad address");      
    exit(1);
  }

  // Установка порта сервера (преобразование в сетевой порядок байт)
  servaddr.sin_port = htons(atoi(argv[2]));

  // Установка TCP-соединения с сервером
  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");      
    exit(1);
  }

  write(1, "Input message to send\n", 22); 

  // Основной цикл программы: чтение ввода и отправка на сервер
  while ((nread = read(0, buf, bufsize)) > 0) {
    // Отправка данных через установленное TCP-соединение
    if (write(fd, buf, nread) < 0) {
      perror("write");          
      exit(1);
    }

  }


  close(fd);
  exit(0); 
}