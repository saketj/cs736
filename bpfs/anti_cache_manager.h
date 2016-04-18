/**
 * Copyright 2016
 **/

#ifndef ANTI_CACHE_MANAGER_H
#define ANTI_CACHE_MANAGER_H

#include <pthread.h>

#define ANTI_CACHE_INIT_SUCCESS 0
#define ANTI_CACHE_INIT_FAILURE 1

#define ANTI_CACHE_DESTROY_SUCCESS 0
#define ANTI_CACHE_DESTROY_FAILURE 1

#define ANTI_CACHE_INVOCATION_INTERVAL 1000000  // in microseconds.

struct lru_t {

};

struct anti_cache_manager_state_t {
  struct lru_t *lru;
};

int anti_cache_manager_init(void);

int anti_cache_manage_access(int blockno);

int anti_cache_manager_destroy(void);

#endif
