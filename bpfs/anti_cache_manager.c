/**
 * Copyright 2016
 **/

#include "anti_cache_manager.h"
#include "lru_hash_map_interface.h"
#include "bpfs.h"
#include "diskmanager.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
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

void lru_push(lru_node_t *node) {
	// Remove the prev and next pointers for the node.
	node->_prev_node = NULL;
	node->_next_node = NULL;
	if (state->_lru->_num_elements == 0) {
		state->_lru->_lru_linked_list->_head = node;
		state->_lru->_lru_linked_list->_tail = node;
		lru_hash_map_insert(state->_lru->_lru_hash_map, node->_blockno,
				(void *) node);
		state->_lru->_num_elements += 1;
	} else {
		// Check if the element is already present in the list.
		lru_node_t *node_p = (lru_node_t *) lru_hash_map_find(
				state->_lru->_lru_hash_map, node->_blockno);
		if (node_p != NULL) {
			if (node_p == state->_lru->_lru_linked_list->_head) {
				// The accessed node is already at the head of the list, do nothing.
				return;
			} else {
				assert(node_p->_prev_node != NULL);  // Make sure node_p is not head.
				// Adjust pointers for the nodes around the requested node.
				node_p->_prev_node->_next_node = node_p->_next_node;
				if (node_p->_next_node != NULL) {
					// Only to be done if the accessed node is not the tail.
					node_p->_next_node->_prev_node = node_p->_prev_node;
				} else {
					// Only to be done if the accessed node is the tail.
					state->_lru->_lru_linked_list->_tail = node_p->_prev_node;
				}
				// Move the existing node to the head of the list.
				node_p->_next_node = state->_lru->_lru_linked_list->_head;
				state->_lru->_lru_linked_list->_head->_prev_node = node_p;
				state->_lru->_lru_linked_list->_head = node_p;
				state->_lru->_lru_linked_list->_head->_prev_node = NULL;
			}
		} else {
			// Add the new node to the head of the list.
			node->_next_node = state->_lru->_lru_linked_list->_head;
			state->_lru->_lru_linked_list->_head->_prev_node = node;
			state->_lru->_lru_linked_list->_head = node;
			state->_lru->_lru_linked_list->_head->_prev_node = NULL;
			lru_hash_map_insert(state->_lru->_lru_hash_map, node->_blockno,
					(void *) node);
			state->_lru->_num_elements += 1;
		}
	}
}

lru_node_t* lru_pop() {
	// When there are no elements to pop.
	if (state->_lru->_num_elements == 0) {
		return NULL;
	}
	// When there is only one element to pop.
	if (state->_lru->_num_elements == 1) {
		lru_node_t *node = state->_lru->_lru_linked_list->_tail;
		state->_lru->_lru_linked_list->_tail = NULL;
		state->_lru->_lru_linked_list->_head = NULL;
		state->_lru->_num_elements = 0;
		return node;
	}
	// When there are more than one elements to pop.
	lru_node_t *node = state->_lru->_lru_linked_list->_tail;
	state->_lru->_lru_linked_list->_tail = node->_prev_node;
	state->_lru->_lru_linked_list->_tail->_next_node = NULL;
	lru_hash_map_erase(state->_lru->_lru_hash_map, node->_blockno);
	state->_lru->_num_elements -= 1;
	return node;
}

void print_lru_contents() {
	lru_node_t *node = state->_lru->_lru_linked_list->_head;
	printf("[");
	while (node != state->_lru->_lru_linked_list->_tail) {
		printf("%d, ", node->_blockno);
		node = node->_next_node;
	}
	if (node != NULL) {
		printf("%d", node->_blockno);
	}
	printf("]\n");
}

void lru_init(lru_t **lru) {
	*lru = (lru_t*) malloc(sizeof(lru_t));
	(*lru)->_lru_linked_list = (lru_linked_list_t*) malloc(
			sizeof(lru_linked_list_t));
	(*lru)->_lru_linked_list->_head = NULL;
	(*lru)->_lru_linked_list->_tail = NULL;
	(*lru)->_lru_hash_map = lru_hash_map_init();
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
		//char *filename = "/tmp/1";
		int fs = stat(diskManagerFileName, &st);

		long int size = 0;
		if (fs == 0) {
			size = st.st_size;
			printf("%ld\n", size);
		}

		//int fd = open(filename, O_RDWR);
		//assert(fd > 0);
		char *buf = get_block(block_num);
		int disk_block_no = writeBlock(buf);
		//assert(pwrite(fd, buf, 4096, size) == 4096);
		//assert(close(fd) == 0);

		uint64_t newpos = disk_block_no;
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
				free_block(block_num);
				break;
			}
		}
	}
	return ANTI_CACHE_BLOCK_EVICTION_SUCCESS;
}

int anti_cache_manager_bulk_evict_to_disk(uint64_t* block_num_arr, uint64_t size) {
	char **block_data_arr = (char **) malloc(size * sizeof(char *));
	uint64_t i = 0;

	// Collect the data of all the blocks that have to be moved to disk.
	for (i = 0; i < size; ++i) {
		uint64_t block_num = block_num_arr[i];
		if (indir_mapping[block_num] != 0) {
			char *buf = get_block(block_num);
			block_data_arr[i] = buf;
		} else {
			assert(false); // This should not be triggered.
		}
	}

	uint64_t *disk_block_num_arr = writeBlocksInBulk(block_data_arr, size);

	// Update the block number to disk block numbers in the parent pointer.
	for (i = 0; i < size; ++i) {
		uint64_t block_num = block_num_arr[i];
		uint64_t disk_block_no = disk_block_num_arr[i];

		uint64_t newpos = disk_block_no;
		uint64_t no = 1;
		no = no << 63;
		uint64_t blockno = newpos | no;

		printf("%ld moving...\n", block_num);
		struct bpfs_indir_block *indir = (struct bpfs_indir_block*) get_block(
				indir_mapping[block_num]);
		int j = 0;
		for (j = 0; j < BPFS_BLOCKNOS_PER_INDIR; j++) {
			if (indir->addr[j] == block_num) {
				indir->addr[j] = blockno;
				printf("%ld moved to disk in %ld.\n", block_num, blockno);
				free_block(block_num);
				break;
			}
		}
	}
	free(block_data_arr);
	free(disk_block_num_arr);
	return ANTI_CACHE_BLOCK_EVICTION_SUCCESS;
}

int anti_cache_manager_update_lru() {
	// Move accessed block to lru from staging queue.
	lru_node_t *node = lru_staging_queue_pop();
	int added_blocks_count = 0;
	while (node != NULL) {
		lru_push(node);
		node = lru_staging_queue_pop();
		++added_blocks_count;
	}
	return added_blocks_count;
}

int anti_cache_manager_evict_blocks() {
	uint64_t evicted_blocks_count = 0;
	if (state->_lru->_num_elements > ANTI_CACHE_EVICTION_THRESHOLD) {
		evicted_blocks_count = state->_lru->_num_elements
				- ANTI_CACHE_EVICTION_THRESHOLD;
		if (ANTI_CACHE_BULK_EVICTION_ENABLED) {
			// Logic to do bulk eviction of multiple blocks.
			uint64_t *block_num_arr = (uint64_t *) malloc(sizeof(uint64_t) * evicted_blocks_count);
			lru_node_t *node = lru_pop();
			uint64_t i = 0;
			for (i = 0; i < evicted_blocks_count && node != NULL; ++i) {
				// Store the block no in the array and free the current block.
				block_num_arr[i] = node->_blockno;
				free(node);
				node = lru_pop();
			}
			// Pass the stored block number array for bulk eviction.
			anti_cache_manager_bulk_evict_to_disk(block_num_arr, evicted_blocks_count);
			free(block_num_arr);
		} else {
			// Logic to evict one block at a time.
			lru_node_t *node = lru_pop();
			uint64_t i = 0;
			for (i = 0; i < evicted_blocks_count && node != NULL; ++i) {
				// Evict and free the current block.
				anti_cache_manager_evict_to_disk(node->_blockno);
				free(node);
				node = lru_pop();
			}
		}
	}
	return evicted_blocks_count;
}

void *anti_cache_manager_main(void *state) {
	while (1) {
		usleep(ANTI_CACHE_INVOCATION_INTERVAL);
		int added_blocks_count = anti_cache_manager_update_lru();
		printf("Anti cache manager added %d blocks to lru.\n",
				added_blocks_count);
		int evicted_blocks_count = anti_cache_manager_evict_blocks();
		printf("Anti cache manager evicted %d blocks to disk.\n",
				evicted_blocks_count);
		printf("Current LRU contents: ");
		print_lru_contents();
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
	assert((blockno & (1<<63)) == 0);
	lru_node_t *new_node = (lru_node_t *) malloc(sizeof(lru_node_t));
	new_node->_blockno = blockno;
	new_node->_prev_node = NULL;
	new_node->_next_node = NULL;
	lru_staging_queue_push(new_node);
	return ANTI_CACHE_ACCESS_SUCCESS;
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
