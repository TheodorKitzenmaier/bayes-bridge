#pragma once

#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <unordered_map>
#include <mutex>

enum WorkerState {
  kReady = 0,
  kRunning,
  kExited
};

struct Worker {
  WorkerState state;
  uint64_t id;
  pid_t pid;
  char command[0x100];
  char input_file[0x80];
  char prior_file[0x80];
  char output_file[0x80];
  char derived_file[0x80];
};

class WorkerMap {
 public:
  Worker* GetWorker(uint64_t t_id);
  void AddWorker(Worker* t_worker);
  void PopWorker(uint64_t t_id);
  uint64_t AllocateId();
 private:
  std::unordered_map<uint64_t, Worker*> workers;
  std::mutex worker_mutex;
};
