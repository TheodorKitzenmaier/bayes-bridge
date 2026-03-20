#include "worker.h"

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
  this->workers.erase(t_id);
}
