

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

int readBlock(const char *fileName, uint64_t blockNumber, char *buf);
int freeBlock(const char *fileName, uint64_t blockNumber);
int writeBlock(const char fileName, char *buf);
void initializeDiskManager(const char *fileName, uint64_t fileSize, uint64_t blockSize);


const char *diskManagerFileName;

uint64_t fileSize;

uint64_t totalBlockCount;

static uint64_t currentDiskBlockNumber=0;

uint64_t diskManagerBlockSize;

struct freeListNode
{
	uint64_t blockNumber;
	struct freeListNode *next;
};

static struct freeListNode *freeList=NULL;

#endif
//Fragmentation Avoidance Part



