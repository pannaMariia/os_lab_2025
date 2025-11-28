#include <netinet/in.h>  
#include <stdio.h>    
#include <stdlib.h>    
#include <string.h>     
#include <sys/socket.h> 
#include <sys/types.h>   
#include <unistd.h>       

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("usage: %s <port> <bufsize>\n", argv[0]);
    exit(1);
  }


  int port = atoi(argv[1]);  
  int bufsize = atoi(argv[2]);   
  char buf[bufsize];            
  
  const size_t kSize = sizeof(struct sockaddr_in);  // Размер структуры адреса

  int lfd, cfd;                  // Дескрипторы сокетов: lfd - listening, cfd - client
  int nread;                     // Количество прочитанных байт
  struct sockaddr_in servaddr;   // Структура для адреса сервера
  struct sockaddr_in cliaddr;    // Структура для адреса клиента

  // Создание TCP сокета 
  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");          
    exit(1);
  }

  // Инициализация структуры адреса сервера нулями
  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;        
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
  servaddr.sin_port = htons(port);        

  // Привязка сокета к адресу и порту
  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");            
    exit(1);
  }

  // Перевод сокета в режим прослушивания
  if (listen(lfd, 5) < 0) {
    perror("listen");            
    exit(1);
  }

  // Бесконечный цикл обработки клиентских соединений
  while (1) {
    unsigned int clilen = kSize;  // Длина структуры адреса клиента

    // Программа ждет пока клиент не подключится
    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");     
      exit(1);
    }
    
    printf("connection established\n");

    // Цикл чтения данных от подключенного клиента
    while ((nread = read(cfd, buf, bufsize)) > 0) {
      // Вывод полученных данных на стандартный вывод 
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");       
      exit(1);
    }
    

    close(cfd);
  }

}