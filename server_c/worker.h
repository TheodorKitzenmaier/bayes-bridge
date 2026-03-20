#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>

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
};


