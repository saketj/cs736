/**
 * Copyright 2016
 **/

#include "anti_cache_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "bpfs.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>

static pthread_t anti_cache_manager_thread;
static struct anti_cache_manager_state_t *anti_cache_manager_state;

int move_block_to_disk(int i) {

	if (indir_mapping[candidate_blocks[i]] != 0) {
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
		char *buf = get_block(candidate_blocks[i]);
		assert(pwrite(fd, buf, 4096, size) == 4096);
		assert(close(fd) == 0);

		uint64_t newpos = size / 4096;
		uint64_t no = 1;
		no = no << 63;
		uint64_t blockno = newpos | no;
		uint64_t blockno_bt = blockno ^ no;
		assert(blockno_bt == newpos);

		printf("%ldMoving \n", candidate_blocks[i]);
		struct bpfs_indir_block *indir = (struct bpfs_indir_block*) get_block(
				indir_mapping[candidate_blocks[i]]);
		int j = 0;
		for (j = 0; j < BPFS_BLOCKNOS_PER_INDIR; j++) {
			if (indir->addr[j] == candidate_blocks[i]) {
				indir->addr[j] = blockno;
				printf("%ldMoving to disk in %ld \n", candidate_blocks[i],
						blockno);
				candidate_blocks[i] = -1;
				break;
			}
		}
	}
	return 0;

}

void *anti_cache_manager_main(void *state) {
	while (1) {
		usleep(ANTI_CACHE_INVOCATION_INTERVAL);
		printf("Anti cache manager does something interesting.\n");

		int no_candidates = sizeof(candidate_blocks)
				/ sizeof(candidate_blocks[0]);
		int i = 0;
		for (i = 0; i < no_candidates; i++) {
			if (candidate_blocks[i] != -1) {

				move_block_to_disk(i);
				break;
			}
		}
	}
}

int anti_cache_manager_init(void) {
	anti_cache_manager_state = malloc(
			sizeof(struct anti_cache_manager_state_t));
	int ret = pthread_create(&anti_cache_manager_thread, NULL,
			anti_cache_manager_main, (void *) anti_cache_manager_state);
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
