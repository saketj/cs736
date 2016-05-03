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
const int BLOCK_READ_SIZE = 4096;
const int BPFS_BLOCK_SIZE = 4096;
const int NUM_TRIALS = 100;

void run_bpfs_benchmark(string file_name, int file_size, int anti_cache_size, int hotness_factor);
void run_ext4_benchmark(string file_name, int file_size);

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
  if (fs_type.compare("bpfs") == 0) {
    int anti_cache_size = stoi(argv[4]);
    int hotness_factor = stoi(argv[5]);
    run_bpfs_benchmark(file_name, file_size, anti_cache_size, hotness_factor);
  } else if (fs_type.compare("ext4") == 0) {
    run_ext4_benchmark(file_name, file_size);
  } else {
    cout<<"Unknown filesystem type "<< fs_type <<". \n";
    exit(0);
  }
  return 0;
}

void run_bpfs_benchmark(string file_name, int file_size, int anti_cache_size, int hotness_factor) {
  char buf[BLOCK_READ_SIZE];
  int in_fd = open(file_name.c_str(), O_RDONLY);
  int offset_max = file_size - BLOCK_READ_SIZE;
  int blocks_in_cache = anti_cache_size / BPFS_BLOCK_SIZE;

  vector<int> offset_in_cache;

  // Warm-up Cache.
  printf("Warming up the anti-cache with %d blocks...\n", blocks_in_cache);
  for (int i = 0; i < blocks_in_cache; ++i) {
    int offset = rand() % offset_max;
    ssize_t result = pread(in_fd, &buf[0], sizeof(buf), offset);
    offset_in_cache.push_back(offset);
  }
  cout<<"Anti-cache has been warmed-up with hot data.\n";

  long cumm_time = 0;

  for (int i = 0; i < NUM_TRIALS; ++i) {
    printf("BPFS Benchmark: Trial %i\n", i+1);

    int offset;
    // Bias based on degree of hotness.
    int bias = rand() % 100;
    if (bias < hotness_factor) {
      // Read from anti-cache.
      while(1) {
        offset = rand() % offset_max;
        // Offset should be in the offset_in_cache.
        if (find(offset_in_cache.begin(), offset_in_cache.end(), offset) != offset_in_cache.end()) {
          break;
        }
      }
    } else {
      // Read from disk.
      while(1) {
        offset = rand() % offset_max;
        // Offset should not be in the offset_in_cache.
        if (find(offset_in_cache.begin(), offset_in_cache.end(), offset) == offset_in_cache.end()) {
          // Now it should put in the offset_in_cache, once it has been read.
          offset_in_cache.push_back(offset);
          break;
        }
      }
      clear_disk_cache();
    }

    long begin = get_high_precision_real_time();
    ssize_t result = pread(in_fd, &buf[0], sizeof(buf), offset);
    long end = get_high_precision_real_time();

    long run_time = (end - begin);
    assert(run_time > 0);
    cumm_time += (run_time);
  }

  double cumm_time_in_ms = (double) cumm_time / (double) (1000000);

  printf("BPFS benchmark time: %f milliseconds.\n", cumm_time_in_ms);
}

void run_ext4_benchmark(string file_name, int file_size) {
  char buf[BLOCK_READ_SIZE];
  int in_fd = open(file_name.c_str(), O_RDONLY);
  int offset_max = file_size - BLOCK_READ_SIZE;

  long cumm_time = 0;

  for (int i = 0; i < NUM_TRIALS; ++i) {
    off_t offset = rand() % offset_max;
    clear_disk_cache();

    printf("Ext4 Benchmark: Trial %i\n", i+1);

    long begin = get_high_precision_real_time();
    ssize_t result = pread(in_fd, &buf[0], sizeof(buf), offset);
    long end = get_high_precision_real_time();

    long run_time = (end - begin);
    assert(run_time > 0);
    cumm_time += (run_time);
  }

  double cumm_time_in_ms = (double) cumm_time / (double) (1000000);

  printf("Ext4 benchmark time: %f milliseconds.\n", cumm_time_in_ms);
}
