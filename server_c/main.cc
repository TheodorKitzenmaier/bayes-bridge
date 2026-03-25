#include <sys/socket.h>
#include <cstring>
#include <cstdio>
#include <list>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>

#include "worker.h"
#include "request_handler.h"

int soc = 0;
WorkerMap* workers;
std::list<RequestHandler*>* handlers;

void shutdown(int signo, siginfo_t* info, void* context) {
  printf("Terminating...\n");
  for (auto handler : *handlers) {
    delete handler;
  }
  delete workers;
  if (soc) {
    close(soc);
  }
  printf("Goodbye!\n");
  _exit(0);
}

int main(int argc, char** argv) {
  // TODO read args for thread count, port, etc.
  int port = 5117;
  int thread_count = 10;

  workers;
  handlers = new std::list<RequestHandler*>();

  printf("Initializing thread handlers.\n");
  for (int i = 0; i < thread_count; i++) {
    handlers->push_back(new RequestHandler());
  }
  for (auto handler : *handlers) {
    handler->Ready();
    handler->Run(workers);
  }

  printf("Setting SIGINT handler.\n");
  struct sigaction handle_exit {0};
  handle_exit.sa_flags = SA_SIGINFO;
  handle_exit.sa_sigaction = &shutdown;
  if (sigaction(SIGINT, &handle_exit, NULL) == -1) {
    printf("Failed to set interrupt handler.\n");
    shutdown(0, nullptr, nullptr);
  };
  printf("Press Ctrl+C to terminate the server.\n");

  printf("Setting up socket to listen to requests.\n");
  soc = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  bind(soc, (sockaddr*)&server_addr, sizeof(sockaddr_in));
  listen(soc, 64);

  printf("Ready accept requests.\n");
  sockaddr_in client_addr;
  socklen_t client_addr_size;
  std::list<RequestHandler*>::iterator current_handler = handlers->begin();
  while(true) {
    int fd = accept(soc, (sockaddr*)&client_addr, &client_addr_size);
    (*current_handler)->EnqueueFd(fd);
    current_handler++;
    if (current_handler == handlers->end()) {
      current_handler = handlers->begin();
    }
  }
}
