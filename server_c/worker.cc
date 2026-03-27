#include <cstdio>
#include <signal.h>

#include "worker.h"

WorkerMap::WorkerMap():
    worker_mutex(),
    workers() {
}

WorkerMap::~WorkerMap() {
  // Don't start a zombie apocalypse.
  for(auto worker: workers) {
    if (worker.second->pid) {
      kill(worker.second->pid, SIGKILL);
      waitpid(worker.second->pid, nullptr, 0);
    }
    delete worker.second;
  }
}

Worker* WorkerMap::GetWorker(uint64_t t_id) {
  const std::lock_guard<std::mutex> lock(this->worker_mutex);
  if (auto worker = this->workers.find(t_id); worker != this->workers.end())
    return worker->second;
  return nullptr;
}

void WorkerMap::AddWorker(Worker* t_worker) {
  const std::lock_guard<std::mutex> lock(this->worker_mutex);
  this->workers[t_worker->id] = t_worker;
}

void WorkerMap::PopWorker(uint64_t t_id) {
  const std::lock_guard<std::mutex> lock(this->worker_mutex);
  delete this->workers[t_id];
  this->workers.erase(t_id);
}

uint64_t WorkerMap::AllocateId() {
  const std::lock_guard<std::mutex> lock(this->worker_mutex);
  FILE* random = fopen("/dev/urandom", "r");
  printf("%p\n", random);
  uint64_t new_id;
  auto worker = this->workers.find(0);
  do {
    fscanf(random, "%8c", (char*)&new_id);
    worker = this->workers.find(new_id);
  } while (worker != this->workers.end());
  this->workers[new_id] = nullptr;
  fclose(random);
  return new_id;
}
