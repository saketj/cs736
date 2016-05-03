#include "diskmanager.h"
#include "lru_hash_map_interface.h"
#include "debug_print.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

static int shm_fd;
static void *ptr;
static const int SIZE = 8;
static const char *name = "/smflag";

// Not open and close file every time. This will cause the file to flush
// Have a way of moving multiple blocks to and fro disk

const char *DISK_CACHE_CLEAR_CMD = "sync ; sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'";
void clear_disk_cache() {
  int i = system(DISK_CACHE_CLEAR_CMD);
  if (i != 0) {
    printf("Disk cache clear command failed with status %d\n", i);
  }
}


void initializeDiskManager(char *fileName, uint64_t size, uint64_t blockSize) {
	//Dprintf(
	//		"Initializing Disk Manager with filename %s, size of file: %ld and block size: %ld\n",
	//		fileName, size, blockSize);
	diskManagerFileName = strdup(fileName);
	fileSize = size;
	totalBlockCount = fileSize / blockSize;
	diskManagerBlockSize = blockSize;
	prefetch_hash_map = lru_hash_map_init();

	const char *name = "/smflag";
	/* create the shared memory segment */
	shm_fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
	if (shm_fd < 0)
    {
        printf("In shm_open() %d\n", shm_fd);
        exit(1);
    }

	/* configure the size of the shared memory segment */
	ftruncate(shm_fd,SIZE);

	/* now map the shared memory segment in the address space of the process */
	ptr = mmap(0,SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Map failed\n");
		return -1;
	}

	// Set sync to false.
	int *shm = (int *) ptr;
	*shm = 0;

	//Dprintf("Completed Initializing Disk Manager\n");

}

//TODO (Siddharth) - Assert statements everywhere to verify input, size of buffer.etc

int readBlocksWithPrefetch(uint64_t blockNumber, char *buf) {

	//Dprintf("Inside Disk Manager readBlockswithprefetch for blockNumber: %ld\n",
		//	blockNumber);
	int *shm = (int *) ptr;
	printf("here %d", *shm);
	if (*shm != 0) {
		printf("enabled\n");
		clear_disk_cache();
	}

	if (buf == NULL)
		return -1;
	// if block has not been prefetched
	char *block;
	if ((block = (char *) lru_hash_map_find(prefetch_hash_map, blockNumber))) {
		if (block != NULL) {
			memcpy(buf, block, diskManagerBlockSize);
			return 1;
		}
	}
	//Dprintf(
	//		"After checking prefetched info inside readblockswithprefetch: %ld\n",
	//		blockNumber);

	int fd = open(diskManagerFileName, O_RDWR);

	char *tempbuf = (char *) malloc(
	READ_PREFETCH_BLOCK_COUNT * diskManagerBlockSize);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;

	uint64_t maxPrefetch = READ_PREFETCH_BLOCK_COUNT;
	if ((currentDiskBlockNumber - 1)
			< blockNumber + READ_PREFETCH_BLOCK_COUNT - 1) {
		maxPrefetch = currentDiskBlockNumber - blockNumber;
	}

	//Dprintf("maxprefetch: %ld currentDiskBlockNumber:%ld blockNumber:%ld\n",
	//		maxPrefetch, currentDiskBlockNumber, blockNumber);
	assert(
			pread(fd, tempbuf, maxPrefetch * diskManagerBlockSize, offset)
					== maxPrefetch * diskManagerBlockSize);
	memcpy(buf, tempbuf, diskManagerBlockSize);
	buf[diskManagerBlockSize] = '\0';
	//Dprintf("copied data into buffer inside readBlocksWithPrefetch: %ld\n",
	//		blockNumber);
	//TODO (Siddharth) - What if after prefetching block is updated.
	int i = 0;

	for (; i < maxPrefetch; i++) {
		//Dprintf("Inserting the %d block into the hash map", i);
		lru_hash_map_insert(prefetch_hash_map, blockNumber + i,
				(void *) (tempbuf + i * diskManagerBlockSize));
	}

	assert(close(fd) == 0);
	//Dprintf("Completed reading block number %ld and contents are %s\n",
	//		blockNumber, buf);
	return 1;

}

int readBlock(uint64_t blockNumber, char *buf) {

	//Dprintf("Inside Disk Manager readBlock for blockNumber: %ld\n", blockNumber);
	int fd = open(diskManagerFileName, O_RDWR);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(
			pread(fd, buf, diskManagerBlockSize, offset)
					== diskManagerBlockSize);
	assert(close(fd) == 0);
	//Dprintf("Completed reading block number %ld and contents are %s\n",
		//	blockNumber, buf);
	return 1;

}

void writeBlocksInBulk(char **buf, uint64_t *returnBlockNumbers, uint64_t numberOfBlocks) {

	//Dprintf("Inside Disk Manager writeBlocksInBulk\n");
	int i = 0;
	int fd = open(diskManagerFileName, O_RDWR);
	for (; i < numberOfBlocks; i++) {
	//	Dprintf("Calling findfreeblock for block: %d\n", i);
		uint64_t blockNumber = findFreeBlock();
		if (blockNumber == -1) {
			return NULL; //failure to write block. couldnt find free block
		}
	//	Dprintf("New block number from findfreeblock for block %d: %ld\n", i,
	//			blockNumber);
		uint64_t offset = (blockNumber) * diskManagerBlockSize;
		char *tempbuf = *(buf + i);
		assert(
				pwrite(fd, tempbuf, diskManagerBlockSize, offset)
						== diskManagerBlockSize);
		*(returnBlockNumbers + i) = blockNumber;

	}
	assert(close(fd) == 0);
	//Dprintf("Completed writing in bulk to disk\n");
}

void addBlockToFreeList(uint64_t blockNumber) {
	//Dprintf("Inside addBlockToFreeList: %ld\n", blockNumber);
	struct freeListNode *temp = (struct freeListNode *) malloc(
			sizeof(struct freeListNode));
	temp->blockNumber = blockNumber;
	temp->next = NULL;
	if (freeList == NULL) {

		freeList = temp;
		return;
	}
	temp->next = freeList;
	freeList = temp;
}

uint64_t getBlockFromFreeList() {

	if (freeList == NULL) {
		return -1;
	}
	//Dprintf("Inside disk manager getBlockFromFreeList\n");
	uint64_t bNumber = freeList->blockNumber;
	struct freeListNode *temp = freeList;
	freeList = freeList->next;
	free(temp);
	//Dprintf("FreeList block number: %ld", bNumber);
	return bNumber;
}

int freeBlock(uint64_t blockNumber) {

	//Dprintf("Inside disk manager freeBlock with block number %ld\n",
	//		blockNumber);
	char block[diskManagerBlockSize];
	int i;
	for (i = 0; i < diskManagerBlockSize; i++)
		block[i] = "";

	int fd = open(diskManagerFileName, O_RDWR);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(
			pwrite(fd, block, diskManagerBlockSize, offset)
					== diskManagerBlockSize);
	//Now modify the prefetch_hash_map also
	char *blockAddress;
	if ((blockAddress = (char *) lru_hash_map_find(prefetch_hash_map,
			blockNumber))) {
		if (blockAddress != NULL) {
			lru_hash_map_erase(prefetch_hash_map, blockNumber);
			free(blockAddress);
		}
	}
	assert(close(fd) == 0);
	//Dprintf("Completed freeing block on disk\n");
	addBlockToFreeList(blockNumber);
	return 1;
}

int writeBlock(char *buf) {
	//Dprintf("Inside Disk Manager writeBlock\n");
	//Dprintf("Calling findfreeblock\n");
	uint64_t blockNumber = findFreeBlock();
	if (blockNumber == -1) {
		return -1; //failure to write block. couldnt find free block
	}
	//Dprintf("New block number from findfreeblock: %ld\n", blockNumber);
	int fd = open(diskManagerFileName, O_RDWR);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(
			pwrite(fd, buf, diskManagerBlockSize, offset)
					== diskManagerBlockSize);
	assert(close(fd) == 0);
	//Dprintf("Completed writing to disk\n");
	return blockNumber;
}

int findFreeBlock() {

	//Dprintf("Inside Disk Manager findFreeBlock\n");
	uint64_t blockNumber;
	if (currentDiskBlockNumber >= (totalBlockCount - 1)) {
	//	Dprintf(
	//			"current block number has exceeded disk space.. Fetching blocks from free list\n");
		uint64_t freeListBlockNumber = getBlockFromFreeList();
		if (freeListBlockNumber == -1) {
	//		Dprintf("No free blocks on disk\n");
			return -1;  //No space in file to allocate
		}

		blockNumber = freeListBlockNumber;
	} else {
	//	Dprintf("current block number: %ld\n", currentDiskBlockNumber);
		blockNumber = currentDiskBlockNumber;
		currentDiskBlockNumber++;
	}

	//Dprintf("Free block number: %ld\n", blockNumber);
	return blockNumber;
}
