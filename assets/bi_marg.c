#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

struct priors {
  double decay_rate_1;
  double decay_rate_2;
  double amplitude_1;
  double amplitude_2;
  double constant;
};

int main (int argc, char** argv) {
  char* in;
  char* pr;
  char* out;
  char* drv;

  for (int i = 0; i < argc - 1; i++) {
    if (!strcmp("-i", argv[i])) {
      in = argv[i+1];
    }
    if (!strcmp("-p", argv[i])) {
      pr = argv[i+1];
    }
    if (!strcmp("-o", argv[i])) {
      out = argv[i+1];
    }
    if (!strcmp("-d", argv[i])) {
      drv = argv[i+1];
    }
  }

  FILE* in_file = fopen(in, "r");
  FILE* pr_file = fopen(pr, "r");
  struct stat st;
  stat(in, &st);
  int count = st.st_size / 8;
  double* abscissa = (double*)malloc(count * sizeof(double));
  double* signal = (double*)malloc(count * sizeof(double) * 3);
  fread(abscissa, sizeof(double), count, in_file);
  fclose(in_file);
  struct priors priors;
  fread(&priors, sizeof(struct priors), 1, pr_file);
  fclose(pr_file);

  FILE* drv_file = fopen(drv, "w");
  double drv_val[6];
  drv_val[0] = priors.decay_rate_1;
  drv_val[1] = priors.decay_rate_2;
  drv_val[2] = priors.decay_rate_2;
  drv_val[3] = priors.decay_rate_1 == 0.0 ? 0.0 : 1.0 / priors.decay_rate_1;
  drv_val[4] = priors.decay_rate_2 == 0.0 ? 0.0 : 1.0 / priors.decay_rate_2;
  drv_val[5] = priors.decay_rate_2 == 0.0 ? 0.0 : 1.0 / priors.decay_rate_2;
  fwrite(drv_val, sizeof(double), 6, drv_file);
  fclose(drv_file);

  int error_1 = 1;
  int error_2 = 1;
  for (int i = 0; i < count; i++) {
    signal[i] = exp(-priors.decay_rate_1 * abscissa[i]);
    if (signal[i] != 0.0) {
      error_1 = 0;
    }
    
    signal[i + count] = exp(-priors.decay_rate_2 * abscissa[i]);
    if (signal[i + count] != 0.0) {
      error_2 = 0;
    }

    signal[i + 2 * count] = 1.0;
  }
  if (error_1) {
    signal[0] = 1.0;
  }
  if (error_2) {
    signal[0 + count] = 1.0;
  }
  FILE* out_file = fopen(out, "w");
  fwrite(signal, sizeof(double), count * 3, out_file);
  fclose(out_file);

  free(abscissa);
  free(signal);

  return 0;
}
