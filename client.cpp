#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#define MAX_EV 2
#define HOST "127.0.0.1"

int main(int argc, char *argv[]) {
  /* Set the client name passed from the command line */
  std::string client_name = "Bob";
  if (argc > 1) {
    client_name = std::string(argv[1]);
    std::cout << "Client name is: " << client_name << '\n';
  }

  /* Create network socket */
  int port = 3300;
  struct sockaddr_in server_addr;
  struct in_addr ipv4addr;
  struct hostent *hp;

  server_addr.sin_family = AF_INET;
  inet_pton(AF_INET, HOST, &ipv4addr);
  hp = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
  bcopy(hp->h_addr, &(server_addr.sin_addr.s_addr), hp->h_length);

  int client = socket(AF_INET, SOCK_STREAM, 0);
  if (client < 0) {
    std::cout << "Failed to create client socket!\n";
    exit(1);
  }

  /* Connect to the server */
  server_addr.sin_port = htons(port); 

  if (connect(client, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
    std::cout << "Failed to connect to server!\n";
    exit(1);
  }
  /* Create epoll */ 
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    std::cout << "Failed to create epoll!\n";
    exit(1);
  }

  struct epoll_event ev;
  struct epoll_event evlist[MAX_EV];

  /* Add event to read stdin */
  ev.events = EPOLLIN;
  ev.data.fd = 0;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev) < 0) {
    std::cout << "Failed to add stdin to epoll!\n";
    exit(1);
  }
  
  /* Add event to read message from server */
  ev.events = EPOLLIN;
  ev.data.fd = client;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev) < 0) {
    std::cout << "Failed to add client fd to epoll!\n";
    exit(1);
  }

  int buffer_sz = 1024;
  char rx_buf[buffer_sz] = {0};
  int ev_num = 0;
  bool run = true;

  while (run) {
    /* Wait for events */
    ev_num = epoll_wait(epfd, evlist, MAX_EV, -1);
    if (ev_num > 0) {
      for (int i = 0; i < ev_num; i++) {
        memset(rx_buf, 0, buffer_sz);
        /* Print messages from the server */
        if (evlist[i].data.fd == client) {
          recv(client, rx_buf, buffer_sz, 0);
          std::cout << rx_buf << '\n'; 
        } 
        /* Send user input to the server */
        if (evlist[i].data.fd == 0) {
          ssize_t len = read(0, rx_buf, buffer_sz);
          std::string msg = "[" + client_name + "] " + std::string(rx_buf);
          send(client, msg.c_str(), msg.size() , 0);
        }
      }
    }
  }
}
