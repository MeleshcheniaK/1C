#include <arpa/inet.h>
#include <bits/types/sig_atomic_t.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/signum-generic.h>

const int STR_SIZE = 4096;
struct sockaddr_in sockaddr;
volatile sig_atomic_t socket_fd = -1;
volatile sig_atomic_t must_exit = 0;

/// Обработка сигналов остановки
void HandleSigstop(int signum) {
  must_exit = 1;
}

/// Установка маски сигналов
void SetSignalHandler() {
  struct sigaction action_stop;
  memset(&action_stop, 0, sizeof(action_stop));
  action_stop.sa_handler = HandleSigstop;

  sigaction(SIGINT, &action_stop, NULL);
  sigaction(SIGTERM, &action_stop, NULL);
}

/// Выделение сокета для клиента
void SetSocket() {
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd == -1) {
    printf("Can't initialize socket");
    return;
  }
}

/// Установка адреса для клиента
void SetAddress(char* port_num, char* addrnum) {
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(atoi(port_num));
  sockaddr.sin_addr.s_addr = inet_addr(addrnum);
}

/// Активация клиента, ожидание сообщений и их последующая отправка
void ClientFunc() {
  char str[STR_SIZE];
  int connect_ret = connect(socket_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
  if (connect_ret == -1) {
    printf("Can't connect to tcp socket");
    return;
  }
  while (scanf("%s", str) && !must_exit) {
    int written_bytes = write(socket_fd, str, STR_SIZE);
    if (written_bytes <= 0) {
      printf("Can't send to server");
      return;
    }
  }
}

int main(int argc, char* argv[]) {
  SetSignalHandler();

  SetSocket();
  SetAddress(argv[1], argv[2]);
  ClientFunc();

  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);

  return 0;
}
