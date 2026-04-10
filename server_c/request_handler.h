#pragma once

#include <mutex>
#include <list>
#include <thread>

#include "worker.h"

// This class is not meant to be accessed concurrently, but does synchronize with it's underlying thread.
class RequestHandler {
 public:
  RequestHandler();
  RequestHandler(RequestHandler&& other) = delete;
  RequestHandler& operator=(RequestHandler&& other) = delete;
  RequestHandler(const RequestHandler& other) = delete;
  RequestHandler& operator=(const RequestHandler& other) = delete;

  ~RequestHandler();

  // If there is a thread running, indicate it should exit once all requests are handled and join.
  void Join();
  // Set the handler to be ready to accept new file descriptors. Only possible if there is no active thread.
  bool Ready();
  // Attempt to add a file descriptor to handle. Only possible if the worker is ready or running.
  // The handler will handle requests from this fd until it is closed.
  bool EnqueueFd(int fd);
  void Run(WorkerMap* workers);
 private:
  void HandleRequests(WorkerMap* workers);

  bool exit_on_empty_;
  std::mutex lock_;
  std::list<int> fd_queue_;
  std::thread* thread_;
};

// Write response data back into request buffer (size will always be the same).
void ProcessRequest(char* t_request, WorkerMap* workers);
