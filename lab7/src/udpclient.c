#include <netinet/in.h>   
#include <stdio.h>      
#include <stdlib.h>     
#include <arpa/inet.h> 
#include <string.h>    
#include <sys/socket.h>  
#include <sys/stat.h>   
#include <sys/types.h>  
#include <unistd.h>   

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {

  if (argc != 4) {
    printf("usage: %s <IPaddress> <port> <bufsize>\n", argv[0]);
    exit(1);
  }


  int port = atoi(argv[2]);     
  int bufsize = atoi(argv[3]);  
  

  int sockfd, n;                 // sockfd - дескриптор сокета, n - количество прочитанных байт
  char sendline[bufsize], recvline[bufsize + 1];  // Буферы для отправки и приема данных
  struct sockaddr_in servaddr;   // Структура для адреса сервера

  // Инициализация структуры адреса сервера нулями
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;          
  servaddr.sin_port = htons(port);         

  // Преобразование IP-адреса из строкового представления в двоичное
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }

  // Создание UDP сокета 
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");    
    exit(1);
  }

  write(1, "Enter string\n", 13); 

  // Основной цикл программы: чтение ввода и обмен данными с сервером
  while ((n = read(0, sendline, bufsize)) > 0) {  // 0 - стандартный ввод (stdin)
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem"); 
      exit(1);
    }

    // Прием ответа от сервера
    // NULL параметры означают, что нам не нужна информация об отправителе
    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem"); 
      exit(1);
    }

    printf("REPLY FROM SERVER= %s\n", recvline);
  }


  close(sockfd);
  
  return 0; 
}