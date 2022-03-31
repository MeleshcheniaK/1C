#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

volatile sig_atomic_t must_exit = 0;
int client_socket = -1;
int server_fd = -1;
struct sockaddr_in sockaddr;
static int stop_pipe_fds[2] = {-1, -1};
FILE* logs_file;

const int LISTEN_BACKLOG = 128;
const size_t STRING_SIZE = 4096;
const size_t CLIENTS_NUM = 10;

void HandleSigstop(int signum) {
  if (client_socket > -1) {
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    client_socket = -1;
  }

  must_exit = 1;
}

void SetSignalHandler() {
  struct sigaction action_stop;
  memset(&action_stop, 0, sizeof(action_stop));
  action_stop.sa_handler = HandleSigstop;

  sigaction(SIGINT, &action_stop, NULL);
  sigaction(SIGTERM, &action_stop, NULL);

  pipe(stop_pipe_fds);
  fcntl(stop_pipe_fds[1],
        F_SETFL,
        fcntl(stop_pipe_fds[1], F_GETFL, 0)
            | O_NONBLOCK);
}

void AddFdToEpoll(int epoll, int fd) {
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
  struct epoll_event event = {EPOLLET | EPOLLIN, .data.fd = fd};
  epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &event);
}

void GetMessage(int client_fd) {
  char* message = malloc(STRING_SIZE * sizeof(char));
  char* log = malloc(STRING_SIZE * sizeof(char));
  int message_len;

  while ((message_len = recv(client_fd, message, STRING_SIZE, 0)) > 0) {
    message[message_len] = '\0';
    sprintf(log, "Client %d -- %s\n", client_fd, message);
    fwrite(log, 1, strlen(log), logs_file);
    fflush(logs_file);
  }

  free(message);
  free(log);
}

void SetSocket() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == -1){
    printf("Can't initialize socket");
    exit(0);
  }
}

void SetAddress(char* port_num, char* addrnum) {
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(atoi(port_num));
  sockaddr.sin_addr.s_addr = inet_addr(addrnum);
}


int ServerFunc(int stop_fd) {
  int epoll_fd = epoll_create1(0);
  AddFdToEpoll(epoll_fd, stop_fd);
  AddFdToEpoll(epoll_fd, server_fd);

  int bind_ret = bind(server_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
  if(bind_ret == -1){
    printf("Can't bind to unix socket");
    exit(0);
  }
  int listen_ret = listen(server_fd, LISTEN_BACKLOG);
  if(listen_ret == -1){
    printf("Can't listen to unix socket");
    exit(0);
  }

  logs_file = fopen("Clients_logs.txt", "a");
  if(logs_file == NULL) {
    printf("Can't create or open log file");
    exit(0);
  }

  struct epoll_event events[CLIENTS_NUM];
  while (!must_exit) {
    int count_events = epoll_wait(epoll_fd, events, CLIENTS_NUM, -1);
    for (int i = 0; i < count_events; ++i) {
      if (events[i].data.fd == server_fd) {
        int connection_fd = accept(server_fd, NULL, NULL);
        if(connection_fd == -1) {
          printf("Can't accept incoming connection");
          exit(0);
        }
        AddFdToEpoll(epoll_fd, connection_fd);
      } else {
        GetMessage(events[i].data.fd);
      }
    }
  }

  for (int i = 0; i < CLIENTS_NUM; ++i) {
    if (events[i].data.fd != server_fd) {
      shutdown(events[i].data.fd, SHUT_RDWR);
      close(events[i].data.fd);
    }
  }
  fclose(logs_file);
  shutdown(server_fd, SHUT_RDWR);
  close(server_fd);
  close(epoll_fd);

  return 0;
}

int main(int argc, char** argv) {
  SetSignalHandler();

  SetSocket();
  SetAddress(argv[1], argv[2]);
  ServerFunc(stop_pipe_fds[0]);

  close(stop_pipe_fds[0]);
  close(stop_pipe_fds[1]);

  return 0;
}