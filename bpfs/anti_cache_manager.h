/**
 * Copyright 2016
 **/

#ifndef ANTI_CACHE_MANAGER_H
#define ANTI_CACHE_MANAGER_H

#include <inttypes.h>

#define ANTI_CACHE_INIT_SUCCESS 0
#define ANTI_CACHE_INIT_FAILURE 1

#define ANTI_CACHE_DESTROY_SUCCESS 0
#define ANTI_CACHE_DESTROY_FAILURE 1

#define ANTI_CACHE_ACCESS_SUCCESS 0
#define ANTI_CACHE_ACCESS_FAILURE 1

#define ANTI_CACHE_BLOCK_EVICTION_SUCCESS 0
#define ANTI_CACHE_BLOCK_EVICTION_FAILURE 1

#define ANTI_CACHE_INVOCATION_INTERVAL 10 * 1 * 1000  // in microseconds.

#define ANTI_CACHE_EVICTION_THRESHOLD 1 * 10 * 1024  // in units of 4 KB.  

#define ANTI_CACHE_BULK_EVICTION_ENABLED 1

typedef struct lru_node lru_node_t;
typedef struct lru_node_queue lru_node_queue_t;
typedef struct lru lru_t;
typedef struct lru_linked_list lru_linked_list_t;
typedef struct anti_cache_manager_state anti_cache_manager_state_t;

struct lru_node {
	uint64_t _blockno;
	lru_node_t *_prev_node;
	lru_node_t *_next_node;
};

struct lru_node_queue {
	uint64_t _num_elements;
	lru_node_t *_queue_head;
	lru_node_t *_queue_tail;
};

struct lru_linked_list {
	lru_node_t *_head;
	lru_node_t *_tail;
};

struct lru {
	uint64_t _num_elements;
	lru_linked_list_t *_lru_linked_list;
	void *_lru_hash_map;
};

struct anti_cache_manager_state {
	lru_t *_lru;
	lru_node_queue_t *_staging_queue;
};

int anti_cache_manager_init(void);

int anti_cache_manager_access(uint64_t blockno);

int anti_cache_manager_destroy(void);

#endif
