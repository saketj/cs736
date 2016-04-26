
#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int readBlock(uint64_t blockNumber, char *buf);
int freeBlock(uint64_t blockNumber);
int writeBlock(char *buf);
void initializeDiskManager(char *fileName, uint64_t fileSize,
		uint64_t blockSize);

char *diskManagerFileName;

uint64_t fileSize;

#define READ_PREFETCH_BLOCK_COUNT 10

unordered_map<uint64_t,char *> prefetched_blocks
uint64_t totalBlockCount;

static uint64_t currentDiskBlockNumber = 0;

uint64_t diskManagerBlockSize;

struct freeListNode {
	uint64_t blockNumber;
	struct freeListNode *next;
};

static struct freeListNode *freeList = NULL;

#endif
//Fragmentation Avoidance Part

