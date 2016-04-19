/**
 * Copyright 2016
 **/

#include "anti_cache_manager.h"
#include "hash_map.h"
#include "bpfs.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

static pthread_t anti_cache_manager_thread;
static pthread_mutex_t lru_staging_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static anti_cache_manager_state_t *state;

void lru_staging_queue_push(lru_node_t *node) {
	pthread_mutex_lock(&lru_staging_queue_lock);
	if (state->_staging_queue->_queue_head == NULL) {
		// Queue is empty.
		state->_staging_queue->_queue_head = node;
		state->_staging_queue->_queue_tail = node;
	} else {
		// Queue is non-empty.
		state->_staging_queue->_queue_tail->_next_node = node;
		node->_prev_node = state->_staging_queue->_queue_tail;
		state->_staging_queue->_queue_tail = node;
	}
	state->_staging_queue->_num_elements += 1;
	pthread_mutex_unlock(&lru_staging_queue_lock);
}

lru_node_t* lru_staging_queue_pop() {
	lru_node_t *node = NULL;
	pthread_mutex_lock(&lru_staging_queue_lock);
	if (state->_staging_queue->_queue_head == NULL) {
		// Queue is empty.
	} else if (state->_staging_queue->_queue_head
			== state->_staging_queue->_queue_tail) {
		// Only one element in the queue.
		node = state->_staging_queue->_queue_head;
		state->_staging_queue->_queue_head = NULL;
		state->_staging_queue->_queue_tail = NULL;
	} else {
		// Queue has more than one element.
		node = state->_staging_queue->_queue_head;
		state->_staging_queue->_queue_head = node->_next_node;
		state->_staging_queue->_queue_head->_prev_node = NULL;
	}
	if (node != NULL) {
		state->_staging_queue->_num_elements -= 1;
	}
	pthread_mutex_unlock(&lru_staging_queue_lock);
	return node;
}

void lru_init(lru_t **lru) {
	*lru = (lru_t*) malloc(sizeof(lru_t));
	(*lru)->_lru_linked_list = (lru_linked_list_t*) malloc(
			sizeof(lru_linked_list_t));
	(*lru)->_lru_linked_list->_head = NULL;
	(*lru)->_lru_hash_map = hash_map_create_ptr();
}

void lru_staging_queue_init(lru_node_queue_t **staging_queue) {
	*staging_queue = (lru_node_queue_t*) malloc(sizeof(lru_node_queue_t));
	(*staging_queue)->_num_elements = 0;
	(*staging_queue)->_queue_head = NULL;
	(*staging_queue)->_queue_tail = NULL;
}

int anti_cache_manager_evict_to_disk(int block_num) {
	// Evict the blocks to disk here.
	//TODO (Siddharth Suresh) - Disk Manager
	if (indir_mapping[block_num] != 0) {
		struct stat st;
		char *filename = "/tmp/1";
		int fs = stat(filename, &st);

		long int size = 0;
		if (fs == 0) {
			size = st.st_size;
			printf("%ld\n", size);
		}

		int fd = open(filename, O_RDWR);
		assert(fd > 0);
		char *buf = get_block(block_num);
		assert(pwrite(fd, buf, 4096, size) == 4096);
		assert(close(fd) == 0);

		uint64_t newpos = size / 4096;
		uint64_t no = 1;
		no = no << 63;
		uint64_t blockno = newpos | no;
		uint64_t blockno_bt = blockno ^ no;
		assert(blockno_bt == newpos);

		printf("%ldMoving \n", block_num);
		struct bpfs_indir_block *indir = (struct bpfs_indir_block*) get_block(
				indir_mapping[block_num]);
		int j = 0;
		for (j = 0; j < BPFS_BLOCKNOS_PER_INDIR; j++) {
			if (indir->addr[j] == block_num) {
				indir->addr[j] = blockno;
				printf("%ldMoving to disk in %ld \n", block_num, blockno);
				break;
			}
		}
	}
	return ANTI_CACHE_BLOCK_EVICTION_SUCCESS;
}

int anti_cache_manager_evict_blocks() {
	int evicted_blocks_count = 0;
	pthread_mutex_lock(&lru_staging_queue_lock);
	if (state->_staging_queue->_num_elements > ANTI_CACHE_EVICTION_THRESHOLD) {
		evicted_blocks_count = state->_staging_queue->_num_elements
				- ANTI_CACHE_EVICTION_THRESHOLD;
		lru_node_t *current = state->_staging_queue->_queue_head;
		int i = 0;
		for (i = 0; i < evicted_blocks_count; ++i) {
			// Update the head to point the next node after current.
			state->_staging_queue->_queue_head = current->_next_node;
			state->_staging_queue->_queue_head->_prev_node = NULL;
			// Evict and free the current block.
			anti_cache_manager_evict_to_disk(current->_blockno);
			free(current);
			state->_staging_queue->_num_elements--;
			// Update the current.
			current = state->_staging_queue->_queue_head;
		}
	}
	pthread_mutex_unlock(&lru_staging_queue_lock);
	return evicted_blocks_count;
}

void *anti_cache_manager_main(void *state) {
	while (1) {
		usleep(ANTI_CACHE_INVOCATION_INTERVAL);
		int evicted_blocks_count = anti_cache_manager_evict_blocks();
		printf("Anti cache manager evicted %d blocks.\n", evicted_blocks_count);
	}
}

int anti_cache_manager_init(void) {
	// Initialize anti-cache manager state.
	state = (anti_cache_manager_state_t *) malloc(
			sizeof(anti_cache_manager_state_t));
	lru_init(&state->_lru);
	lru_staging_queue_init(&state->_staging_queue);

	// Create the anti-cache manager thread.
	int ret = pthread_create(&anti_cache_manager_thread,
	NULL, anti_cache_manager_main, (void *) state);
	if (ret) {
		return ANTI_CACHE_INIT_FAILURE;
	} else {
		return ANTI_CACHE_INIT_SUCCESS;
	}
}

int anti_cache_manager_access(uint64_t blockno) {
	lru_node_t *new_node = (lru_node_t *) malloc(sizeof(lru_node_t));
	new_node->_blockno = blockno;
	new_node->_prev_node = NULL;
	new_node->_next_node = NULL;
	lru_staging_queue_push(new_node);
	return 0;
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
