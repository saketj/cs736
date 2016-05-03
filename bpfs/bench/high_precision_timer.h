#include <time.h>

long get_high_precision_time() {
  struct timespec time1;
  clock_gettime(CLOCK_MONOTONIC, &time1);
  return time1.tv_nsec;
}

long get_high_precision_monotonic_time() {
  struct timespec time1;
  clock_gettime(CLOCK_MONOTONIC, &time1);
  return time1.tv_nsec;
}

long get_high_precision_real_time() {
  struct timespec time1;
  clock_gettime(CLOCK_REALTIME, &time1);
  return time1.tv_nsec;
}
