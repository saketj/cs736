#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "high_precision_timer.h"

using namespace std;

/*
  g++ -lrt -std=c++11 hot_cold_microbench.cpp
  dd if=/dev/urandom of=/tmp/input bs=1M count=500;
  cp /tmp/input /tmp/bpfs/input
  ./a.out ext4 /tmp/input 524288000
  ./a.out bpfs /tmp/bpfs/input 524288000 41943040 40;
*/

const char *DISK_CACHE_CLEAR_CMD = "sync ; sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'";
long BLOCK_READ_SIZE = 4096;
const int BPFS_BLOCK_SIZE = 4096;
const int NUM_TRIALS = 10;

void run_bpfs_benchmark(string file_name, long file_size);
void run_ext4_benchmark(string file_name, long file_size);

void clear_disk_cache() {
  int i = system(DISK_CACHE_CLEAR_CMD);
  if (i != 0) {
    printf("Disk cache clear command failed with status %d\n", i);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cout<<"Improper invocation.\n";
    exit(1);
  }
  srand(0);
  string fs_type = argv[1];
  string file_name = argv[2];
  int file_size = stoi(argv[3]);
  BLOCK_READ_SIZE = stol(argv[4]);
  if (fs_type.compare("bpfs") == 0) {
    run_bpfs_benchmark(file_name, file_size);
  } else if (fs_type.compare("ext4") == 0) {
    run_ext4_benchmark(file_name, file_size);
  } else {
    cout<<"Unknown filesystem type "<< fs_type <<". \n";
    exit(0);
  }
  return 0;
}

void run_bpfs_benchmark(string file_name, long file_size) {
  char buf[BLOCK_READ_SIZE];
  int in_fd = open(file_name.c_str(), O_RDONLY);
  long offset_max = file_size - BLOCK_READ_SIZE;

  long cumm_time = 0;

  for (int i = 0; i < NUM_TRIALS;) {
    off_t offset = rand() % offset_max;
    clear_disk_cache();

    printf("BPFS Benchmark: Trial %i\n", i+1);

    long begin = get_high_precision_real_time();
    ssize_t result = pread(in_fd, &buf[0], sizeof(buf), offset);
    long end = get_high_precision_real_time();

    long run_time = (end - begin);
    if (run_time > 0) {
      cumm_time += (run_time);
      ++i;
    }
  }
  double avg_time = (double) cumm_time / (double) NUM_TRIALS;
  double avg_time_in_ms = (double) avg_time / (double) (1000000);

  printf("BPFS benchmark time: %f milliseconds.\n", avg_time_in_ms);
}

void run_ext4_benchmark(string file_name, long file_size) {
  char buf[BLOCK_READ_SIZE];
  int in_fd = open(file_name.c_str(), O_RDONLY);
  long offset_max = file_size - BLOCK_READ_SIZE;

  long cumm_time = 0;

  for (int i = 0; i < NUM_TRIALS;) {
    off_t offset = rand() % offset_max;
    clear_disk_cache();

    printf("Ext4 Benchmark: Trial %i\n", i+1);

    long begin = get_high_precision_real_time();
    ssize_t result = pread(in_fd, &buf[0], sizeof(buf), offset);
    long end = get_high_precision_real_time();

    long run_time = (end - begin);
    if (run_time > 0) {
      cumm_time += (run_time);
      ++i;
    }
  }
  double avg_time = (double) cumm_time / (double) NUM_TRIALS;
  double avg_time_in_ms = (double) avg_time / (double) (1000000);

  printf("Ext4 benchmark time: %f milliseconds.\n", avg_time_in_ms);
}
