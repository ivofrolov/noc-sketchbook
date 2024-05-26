#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

double GetGaussianRandomValue(double mean, double std_dev) {
  static double spare;
  static bool has_spare = false;

  if (has_spare) {
    has_spare = false;
    return spare * std_dev + mean;
  }

  double s, u, v;
  do {
    u = (rand() / (double) RAND_MAX) * 2.0 - 1.0;
    v = (rand() / (double) RAND_MAX) * 2.0 - 1.0;
    s = u * u + v * v;
  } while (s >= 1.0 || s == 0.0);
  s = sqrt(-2.0 * log(s) / s);

  spare = v * s;
  has_spare = true;

  return std_dev * u * s + mean;
}

void init(void* arena);
void loop(void* arena);
