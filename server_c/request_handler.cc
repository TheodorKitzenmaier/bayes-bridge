#include <sys/socket.h>
#include <cstring>
#include <cstdio>
#include <vector>

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
  if (!std::strcmp(t_init->input_file, ""))
    std::snprintf(t_init->input_file, sizeof(t_init->input_file), "%s%llu.in", kFilePrefix, worker->id);
  if (!std::strcmp(t_init->prior_file, ""))
    std::snprintf(t_init->prior_file, sizeof(t_init->prior_file), "%s%llu.pr", kFilePrefix, worker->id);
  if (!std::strcmp(t_init->output_file, ""))
    std::snprintf(t_init->output_file, sizeof(t_init->output_file), "%s%llu.out", kFilePrefix, worker->id);
  if (!std::strcmp(t_init->derived_file, ""))
    std::snprintf(t_init->derived_file, sizeof(t_init->derived_file), "%s%llu.drv", kFilePrefix, worker->id);

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

// Write response data back into request buffer (size will always be the same).
void ProcessRequest(char* t_request, WorkerMap* workers) {
  RequestHeader* header = (RequestHeader*)t_request;
  switch(header->type) {
   case MessageType::kInit: {
    ProcessInit((Init*)t_request, workers);
    break;
   }

   case MessageType::kStart: {
    ProcessStart((StartData*)t_request, workers);
    break;
   }

   case MessageType::kQuery:{
    ProcessQuery((QueryData*)t_request, workers);
    break;
   }

   case MessageType::kCollect: {
    Worker* worker = workers->GetWorker(header->worker_id);
    if (worker->pid and worker->state == WorkerState::kRunning) {
      waitpid(worker->pid, nullptr, 0);
    }
    workers->PopWorker(worker->pid);
    break;
   }
  }
}
