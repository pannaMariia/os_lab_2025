#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
// Общая структура для вычислений
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};
// Математические функции
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);
// Функции для работы с сетью
int SocketCreate();
void SocketSetReusable(int sockfd);
int SocketBind(int sockfd, int port);
int SocketConnect(int sockfd, const char* ip, int port);
int SocketSendData(int sockfd, const void* data, size_t size);
int SocketReceiveData(int sockfd, void* buffer, size_t size);
// Вспомогательные функции
void PrintError(const char* message);
#endif 