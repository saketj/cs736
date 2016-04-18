/**
 * Copyright 2016
 **/

#include "anti_cache_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_t anti_cache_manager_thread;
static struct anti_cache_manager_state_t *anti_cache_manager_state;

void *anti_cache_manager_main(void *state) {
  while(1) {
    usleep(ANTI_CACHE_INVOCATION_INTERVAL);
    printf("Anti cache manager does something interesting.\n");
  }
}

int anti_cache_manager_init(void) {
  anti_cache_manager_state = malloc(sizeof(struct anti_cache_manager_state_t));
  int ret = pthread_create(&anti_cache_manager_thread, NULL, anti_cache_manager_main, (void *)anti_cache_manager_state);
  if (ret) {
    return ANTI_CACHE_INIT_FAILURE;
  } else {
    return ANTI_CACHE_INIT_SUCCESS;
  }
}

int anti_cache_manager_destroy(void) {
  int ret = pthread_cancel(anti_cache_manager_thread);
  if (ret) {
    return ANTI_CACHE_DESTROY_FAILURE;
  }
  // Wait for the anti_cache_manager thread to terminate.
  pthread_join(anti_cache_manager_thread, NULL);
  return ANTI_CACHE_DESTROY_SUCCESS;
}
