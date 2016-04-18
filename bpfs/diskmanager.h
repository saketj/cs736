#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int readBlock(const char *fileName, uint blockNumber, char *buf, uint blockSize);
int freeBlock(const char *fileName, uint blockNumber, uint blockSize);
int writeBlock(const char fileName, char *buf, uint blockSize);
int initializeDiskManager(const char *fileName, uint fileSize, uint blockSize);


const char *diskManagerFileName;

uint64_t fileSize;

uint64_t totalBlockCount;

uint64_t currentDiskBlockNumber=0;

uint64_t diskManagerBlockSize;

struct freeListNode
{
	uint64_t blockNumber;
	struct freeListNode *next;
};

struct freeListNode *freeList=NULL;

//Fragmentation Avoidance Part



