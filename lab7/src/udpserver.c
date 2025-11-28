#include <arpa/inet.h>   
#include <netinet/in.h> 
#include <stdio.h>     
#include <stdlib.h>     
#include <string.h>     
#include <sys/socket.h>  
#include <sys/stat.h>    
#include <sys/types.h>   
#include <unistd.h>      

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {

  if (argc != 3) {
    printf("usage: %s <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);      
  int bufsize = atoi(argv[2]);  
  char mesg[bufsize];           
  char ipadr[16];        
  
  int sockfd;                    // Дескриптор сокета
  int n;                         // Количество принятых байт
  struct sockaddr_in servaddr;   // Структура для адреса сервера
  struct sockaddr_in cliaddr;    // Структура для адреса клиента

  // Создание UDP сокета 
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");    
    exit(1);
  }

  // Инициализация структуры адреса сервера нулями
  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;         
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Принимать соединения с любого интерфейса
  servaddr.sin_port = htons(port);   

  // Привязка сокета к адресу и порту
  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");    
    exit(1);
  }
  
  printf("SERVER starts...\n"); 


  while (1) {
    unsigned int len = SLEN;     // Длина структуры адреса клиента

    // Прием данных от клиента 
    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");       
      exit(1);
    }
    
    mesg[n] = 0;  


    printf("REQUEST %s      FROM %s : %d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    // Отправка ответа обратно клиенту
    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");     
      exit(1);
    }
  }
  
}