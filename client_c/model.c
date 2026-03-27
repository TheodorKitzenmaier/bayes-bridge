#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum MessageType : uint32_t {
  kInit = 0,
  kStart,
  kQuery,
  kCollect
};

enum WorkerState : uint32_t {
  kReady = 0,
  kRunning,
  kExited
};

struct RequestHeader {
  uint32_t length;
  enum MessageType type;
  uint64_t worker_id;
};

struct Init {
  struct RequestHeader header;
  char input_file[0x80];
  char prior_file[0x80];
  char output_file[0x80];
  char derived_file[0x80];
};

struct StartData {
  struct RequestHeader header;
  char command[0x400];
};

struct QueryData {
  struct RequestHeader header;
  enum WorkerState state;
};

int Connect(char* addr, int port) {
  uint8_t addr_ints[4];
  sscanf(addr, "%hhu.%hhu.%hhu.%hhu", &addr_ints[3], &addr_ints[2], &addr_ints[1], &addr_ints[0]);
  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = htonl(*(uint32_t*)addr_ints);
  server_addr.sin_port = htons(port);
  server_addr.sin_family = AF_INET;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  connect(fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
  return fd;
}

void MakeRequest(int fd, char* t_buffer) {
  struct RequestHeader* header = (struct RequestHeader*)t_buffer;
  send(fd, t_buffer, header->length, 0);
  recv(fd, t_buffer, header->length, MSG_WAITALL);
}

void model_(
    // TODO: Revaluate names of these args.
    int *t_current,
    int *t_num_priors,
    int *t_num_derived,
    int *t_total_values,
    int *t_max_num_value,
    int *t_num_values,
    int *t_num_abscissa,
    int *t_num_vectors,
    double t_priors[],
    double t_derived[],
    double t_abscissa[],
    double t_signal[]) {
  // The command will be run with additional arguments -i <input file> -p <prior file> -o <output file> -d <derived file>
  char* command = "";

  int fd = Connect("127.0.0.1", 5117);

  struct Init init;
  char buffer[0x1000];
  memset(&init, 0, sizeof(struct Init));
  init.header.length = sizeof(struct Init);
  init.header.type = kInit;
  memcpy(buffer, &init, sizeof(struct Init));
  MakeRequest(fd, buffer);
  memcpy(&init, buffer, sizeof(struct Init));

  FILE* input_file = fopen(init.input_file, "w");
  if (!input_file) {
    return;
  }
  fwrite(t_abscissa, sizeof(double), *t_num_abscissa * *t_total_values, input_file);
  fclose(input_file);
  FILE* prior_file = fopen(init.prior_file, "w");
  if (!prior_file) {
    return;
  }
  fwrite(t_priors, sizeof(double), *t_num_priors, prior_file);
  fclose(prior_file);

  struct StartData* start = (struct StartData*)buffer;
  start->header.length = sizeof(struct StartData);
  start->header.type = kStart;
  snprintf(
    start->command,
    sizeof(start->command),
    "%s -i %s -p %s -o %s -d %s",
    command,
    init.input_file,
    init.prior_file,
    init.output_file,
    init.derived_file);
  MakeRequest(fd, buffer);

  struct QueryData* query = (struct QueryData*)buffer;
  query->header.length = sizeof(struct QueryData);
  query->header.type = kQuery;
  struct timespec to_sleep;
  to_sleep.tv_nsec = 10000;
  to_sleep.tv_sec = 0;
  do {
    MakeRequest(fd, buffer);
    nanosleep(&to_sleep, NULL);
  } while(query->state != kExited);

  FILE* output_file = fopen(init.output_file, "r");
  if (output_file) {
    fread(t_signal, sizeof(double), *t_total_values, output_file);
    fclose(output_file);
  }
  FILE* derived_file = fopen(init.derived_file, "r");
  if (derived_file) {
    fread(t_derived, sizeof(double), *t_num_derived, derived_file);
    fclose(derived_file);
  }

  // Remove these before collecting the worker (avoid a VERY unlikely race condition).
  remove(init.input_file);
  remove(init.prior_file);
  remove(init.output_file);
  remove(init.derived_file);

  struct RequestHeader* collect = (struct RequestHeader*)buffer;
  collect->length = sizeof(struct RequestHeader);
  collect->type = kCollect;
  MakeRequest(fd, buffer);
  close(fd);

  return;
}