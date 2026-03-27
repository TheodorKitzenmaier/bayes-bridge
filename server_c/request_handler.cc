#include <sys/socket.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <errno.h>

#include "request_handler.h"
#include "messages.h"

const char* kFilePrefix = "/tmp/";
const char* kWorkerDir = "/tmp/";

void ProcessInit(Init* t_init, WorkerMap* workers) {
  t_init->header.worker_id = workers->AllocateId();

  Worker* worker = new Worker;
  std::memset(worker, 0, sizeof(Worker));
  worker->id = t_init->header.worker_id;
  worker->state = WorkerState::kReady;

  // Generate file names only if they were not supplied.
  if (!std::strcmp(t_init->input_file, "")) {
    printf("I_Gin\n");
    std::snprintf(t_init->input_file, sizeof(t_init->input_file), "%s%lu.in", kFilePrefix, worker->id);
  }
  if (!std::strcmp(t_init->prior_file, "")) {
    printf("I_Gpr\n");
    std::snprintf(t_init->prior_file, sizeof(t_init->prior_file), "%s%lu.pr", kFilePrefix, worker->id);
  }
  if (!std::strcmp(t_init->output_file, "")) {
    printf("I_Gout\n");
    std::snprintf(t_init->output_file, sizeof(t_init->output_file), "%s%lu.out", kFilePrefix, worker->id);
  }
  if (!std::strcmp(t_init->derived_file, "")) {
    printf("I_Gdrv\n");
    std::snprintf(t_init->derived_file, sizeof(t_init->derived_file), "%s%lu.drv", kFilePrefix, worker->id);
  }

  std::strcpy(worker->input_file, t_init->input_file);
  std::strcpy(worker->prior_file, t_init->prior_file);
  std::strcpy(worker->output_file, t_init->output_file);
  std::strcpy(worker->derived_file, t_init->derived_file);
}

void ProcessStart(StartData* t_start, WorkerMap* workers) {
  Worker* worker = workers->GetWorker(t_start->header.worker_id);

  std::vector<char*> tokens = {};
  char command[sizeof(Worker::command)];
  char* token;
  char* rest = command;
  strcpy(command, t_start->command);
  strcpy(worker->command, t_start->command);

  // Unlike the flask server, rely on the client to substitute files for io.
  while (token = strtok_r(rest, " ", &rest)) {
    tokens.push_back(token);
  }
  tokens.push_back(nullptr);

  pid_t pid = fork();
  if (!pid) {
    chdir(kWorkerDir);
    execve(tokens[0], tokens.data(), {nullptr});
  }
  worker->pid = pid;
  worker->state = WorkerState::kRunning;
}

void ProcessQuery(QueryData* t_query, WorkerMap* workers) {
  Worker* worker = workers->GetWorker(t_query->header.worker_id);
  if (!worker->pid) {
    t_query->state = WorkerState::kReady;
    return;
  }
  int status;
  pid_t result = waitpid(worker->id, &status, WNOHANG);
  if (result >= 0 && WIFEXITED(status)) {
    worker->state = WorkerState::kExited;
  }
  t_query->state = worker->state;
}

void ProcessRequest(char* t_request, WorkerMap* workers) {
  RequestHeader* header = (RequestHeader*)t_request;
  switch(header->type) {
   case MessageType::kInit: {
    printf("I\n");
    ProcessInit((Init*)t_request, workers);
    printf("If\n");
    break;
   }

   case MessageType::kStart: {
    printf("S\n");
    ProcessStart((StartData*)t_request, workers);
    printf("Sf\n");
    break;
   }

   case MessageType::kQuery:{
    printf("Q\n");
    ProcessQuery((QueryData*)t_request, workers);
    printf("Qf\n");
    break;
   }

   case MessageType::kCollect: {
    printf("C\n");
    Worker* worker = workers->GetWorker(header->worker_id);
    if (worker->pid and worker->state == WorkerState::kRunning) {
      waitpid(worker->pid, nullptr, 0);
    }
    workers->PopWorker(worker->pid);
    printf("Cf\n");
    break;
   }
  }
}

RequestHandler::RequestHandler():
    exit_on_empty_(false),
    lock_(),
    fd_queue_(),
    thread_(nullptr) {
}

RequestHandler::~RequestHandler() {
  if (thread_) {
    this->Join();
  }
}

void RequestHandler::Join() {
  if (!thread_) {
    return;
  }
  {
    // Lock only for setting the exit flag...
    const std::lock_guard<std::mutex> lock(lock_);
    exit_on_empty_ = true;
  }
  // ...otherwise we would get a deadlock here.
  thread_->join();
  delete thread_;
  thread_ = nullptr;
}

bool RequestHandler::Ready() {
  const std::lock_guard<std::mutex> lock(lock_);
  if (exit_on_empty_ || thread_) {
    return false;
  }
  exit_on_empty_ = false;
  return true;
}

bool RequestHandler::EnqueueFd(int fd) {
  const std::lock_guard<std::mutex> lock(lock_);
  // Don't allow enqueueing new fds if the handler is attempting to stop/stopped.
  if (exit_on_empty_ || !thread_) {
    return false;
  }
  fd_queue_.push_back(fd);
  return true;
}

void RequestHandler::Run(WorkerMap* workers) {
  if(thread_) {
    return;
  }
  exit_on_empty_ = false;
  thread_ = new std::thread(&RequestHandler::HandleRequests, this, workers);
}

void RequestHandler::HandleRequests(WorkerMap* workers) {
  bool should_stop;
  char buffer[0x1000];
  do {
    int fd_count;
    {
      const std::lock_guard<std::mutex> lock(lock_);
      fd_count = fd_queue_.size();
      should_stop = exit_on_empty_ && fd_count == 0;
    }

    // Don't sit around eating cycles if we have nothing to do.
    if (fd_count == 0) {
      if (!should_stop) {
        sched_yield();
      }
      continue;
    }

    int fd;
    {
      const std::lock_guard<std::mutex> lock(lock_);
      fd = fd_queue_.front();
      fd_queue_.pop_front();
    }

    int result = recv(
      fd,
      buffer,
      sizeof(RequestHeader),
      MSG_DONTWAIT | MSG_PEEK | MSG_WAITALL
    );
    if (result == -1) {
      // This is fine, no data or interrupted.
      if (errno == EAGAIN | EWOULDBLOCK | EINTR) {
        const std::lock_guard<std::mutex> lock(lock_);
        fd_queue_.push_back(fd);
      }
      // FUBAR, just try to close it and pray.
      else {
        close(fd);
      }
      continue;
    }
    if (result < sizeof(RequestHeader)) {
      // Disconnect by client.
      printf("D\n");
      close(fd);
      continue;
    }

    RequestHeader* header = (RequestHeader*)buffer;
    recv(
      fd,
      buffer,
      header->length,
      MSG_WAITALL 
    );
    ProcessRequest(buffer, workers);
    send(fd, buffer, header->length, 0);
    {
      std::lock_guard<std::mutex> lock(lock_);
      fd_queue_.push_back(fd);
    }
  } while(!should_stop);
}