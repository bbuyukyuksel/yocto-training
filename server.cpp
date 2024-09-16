#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <list>

#define MAX_EV 1024

int main() { 

  /* Create server socket */
	socklen_t size;
  int port = 3300;
  struct sockaddr_in server_addr;
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket < 0) {
		std::cout << "Could not create server socket!\n";
		exit(1);
	}
 
  int reuse = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
		std::cout << "Could not set socket options!\n";
		exit(1);
	}


  /* Bind server socket */
	server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
    std::cout << "Could not bind!\n";
    exit(1);
  }
  /* Put socket into listen for client connections mode */ 
  if (listen(server_socket, 1) < 0 ) {
    std::cout << "Listen failed with error= " << errno << "\n";
    exit(1);
  }

  int client_socket = 0;
  std::string welcome_msg = "Welcome to the chat!";
  std::list<int> clients;

  /* Create epoll to wait for multiple events */
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    std::cout << "Failed to create epoll!\n";
    exit(1);
  }

  struct epoll_event ev;
  struct epoll_event evlist[MAX_EV];

  /* Add server socket to epoll events */
  ev.events = EPOLLIN;
  ev.data.fd = server_socket;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev) < 0) {
    std::cout << "Failed to add server fd to epoll!\n";
    exit(1);
  }

  int ev_num = 0;
  int buffer_sz = 1024;
  char rx_buf[buffer_sz] = {0};
  bool run = true;

  while ( run ) {
    ev_num = epoll_wait(epfd, evlist, MAX_EV, -1);
    if (ev_num > 0) {
      for (int i = 0; i < ev_num; i++) {
        /* If new client connects, accept the connection,
           send invitation message, add new socket to
           the list of clients and to epoll events */
        if (evlist[i].data.fd == server_socket) {
          client_socket = accept(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), &size);
          if (client_socket > 0) {
            if (send(client_socket, welcome_msg.c_str(), welcome_msg.size(), 0) > 0) {
              clients.push_back(client_socket);
              ev.events = EPOLLIN;
              ev.data.fd = client_socket;
              if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev) < 0) {
                std::cout << "Failed to add client fd to epoll!\n";
                exit(1);
              }
            }      
          }
        } else {
          /* If the message from existing client is received pass to to all other clients */
          int infd = evlist[i].data.fd;
          int len = recv(infd, rx_buf, buffer_sz, 0);
          for (int c : clients) {
            if (c != infd) {
              send(c, rx_buf, len, 0);
            }
          }
          rx_buf[0] = 0;
        }
      }
    } 
  }
}
