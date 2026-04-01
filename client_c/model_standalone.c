#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>

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

  char input_filename[0x80];
  char prior_filename[0x80];
  char output_filename[0x80];
  char derived_filename[0x80];
  char* kFilePrefix = "/tmp/";

  FILE* input_file = NULL;
  FILE* random = fopen("/dev/urandom", "r");
  do {
    uint64_t id;
    fscanf(random, "%8c", (char*)&id);
    sprintf(input_filename, "%s%lu.in", kFilePrefix, id);
    sprintf(prior_filename, "%s%lu.pr", kFilePrefix, id);
    sprintf(output_filename, "%s%lu.out", kFilePrefix, id);
    sprintf(derived_filename, "%s%lu.drv", kFilePrefix, id);
  }
  // Get exclusive rights over this filename.
  while (!(input_file = fopen(input_filename, "wx")));
  fclose(random);

  fwrite(t_abscissa, sizeof(double), *t_num_abscissa * *t_max_num_value, input_file);
  fclose(input_file);
  FILE* prior_file = fopen(prior_filename, "w");
  if (!prior_file) {
    return;
  }
  fwrite(t_priors, sizeof(double), *t_num_priors, prior_file);
  fclose(prior_file);

  pid_t pid;
  while ((pid = fork()) == -1) {
    sleep(1);
  }
  if (!pid) {
    char command_full[0x400];
    char* command_tokenized[0x200];
    sprintf(command_full, "%s -i %s -p %s -o %s -d %s", command, input_filename, prior_filename, output_filename, derived_filename);
    int i = 0;
    char* token;
    char* rest = command_full;
    while (token = strtok_r(rest, " ", &rest)) {
      command_tokenized[i++] = token;
    }
    command_tokenized[i] = NULL;
    char** envp = {NULL};
    execve(command_tokenized[0], command_tokenized, envp);

    char error_filename[0x84];
    sprintf(error_filename, "%s.err", input_filename);
    FILE* error_file = fopen(error_filename, "w");
    fprintf(error_file, "EXECVE FUBAR\n");
    for (int p = 0; p < i; p++) {
      fprintf(error_file, "%s\n", command_tokenized[p]);
    }
    fclose(error_file);
    _exit(-1);
  }
  int status;
  waitpid(pid, &status, 0);

  FILE* output_file = fopen(output_filename, "r");
  if (output_file) {
    fread(t_signal, sizeof(double), *t_num_values * *t_max_num_value * *t_num_vectors, output_file);
    fclose(output_file);
  }
  FILE* derived_file = fopen(derived_filename, "r");
  if (derived_file) {
    fread(t_derived, sizeof(double), *t_num_derived, derived_file);
    fclose(derived_file);
  }

  remove(input_filename);
  remove(prior_filename);
  remove(output_filename);
  remove(derived_filename);

  return;
}