#include "diskmanager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


// Not open and close file every time. This will cause the file to flush
// Have a way of moving multiple blocks to and fro disk

void initializeDiskManager(char *fileName, uint64_t size, uint64_t blockSize) {
	printf("Initializing Disk Manager with filename %s, size of file: %ld and block size: %ld\n",
			fileName, size, blockSize);
	diskManagerFileName = strdup(fileName);
	fileSize = size;
	totalBlockCount = fileSize / blockSize;
	diskManagerBlockSize = blockSize;
	printf("Completed Initializing Disk Manager\n");

}

//TODO (Siddharth) - Assert statements everywhere to verify input, size of buffer.etc
int readBlock(uint64_t blockNumber, char *buf) {
  
  if(buf==NULL) return -1; 
	printf("Inside Disk Manager readBlock for blockNumber: %ld\n", blockNumber);
	int fd = open(diskManagerFileName, O_RDWR);
  char *tempbuf = (char *)malloc(READ_PREFETCH_BLOCK_COUNT*sizeof(char));
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(pread(fd, tempbuf, READ_PREFETCH_BLOCK_COUNT*diskManagerBlockSize, offset)== diskManagerBlockSize);
  memcpy(buf,tempbuf,diskManagerBlockSize);
	assert(close(fd) == 0);
	printf("Completed reading block number %ld and contents are %s\n",
			blockNumber, buf);
	return 1;

}



int *writeBlocksInBulk(char **buf, uint64_t numberOfBlocks)
{

  int *returnBlockNumbers=(int *)malloc(sizeof(int)*numberOfBlocks);
  printf("Inside Disk Manager writeBlocksInBulk\n");
  int i=0;
  int fd = open(diskManagerFileName, O_RDWR);
  for(;i<numberOfBlocks;i++)
  {
  printf("Calling findfreeblock for block: %ld\n",i);
  uint64_t blockNumber = findFreeBlock();
  if (blockNumber == -1) {
    return NULL; //failure to write block. couldnt find free block
  }
  printf("New block number from findfreeblock for block %ld: %ld\n", i,blockNumber);
  uint64_t offset = (blockNumber) * diskManagerBlockSize;
  char *tempbuf=*(buf+i);
  assert(pwrite(fd, tempbuf, diskManagerBlockSize, offset)== diskManagerBlockSize);
  *(returnBlockNumbers+i)=blockNumber; 
  
  }
  assert(close(fd) == 0);
  printf("Completed writing in bulk to disk\n");
  return returnBlockNumbers;

}



void addBlockToFreeList(uint blockNumber) {
	printf("Inside addBlockToFreeList: %ld\n", blockNumber);
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
	printf("Inside disk manager getBlockFromFreeList\n");
	uint64_t bNumber = freeList->blockNumber;
	struct freeListNode *temp = freeList;
	freeList = freeList->next;
	free(temp);
	printf("FreeList block number: %ld", bNumber);
	return bNumber;
}


int freeBlock(uint64_t blockNumber) {

	printf("Inside disk manager freeBlock with block number %ld\n",
			blockNumber);
	char block[diskManagerBlockSize];
	int i;
	for (i = 0; i < diskManagerBlockSize; i++)
		block[i] = "";

	int fd = open(diskManagerFileName, O_RDWR);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(pwrite(fd, block, diskManagerBlockSize, offset)== diskManagerBlockSize);
	assert(close(fd) == 0);
	printf("Completed freeing block on disk\n");
	addBlockToFreeList(blockNumber);
	return 1;

}

int writeBlock(char *buf) {
	printf("Inside Disk Manager writeBlock\n");
	printf("Calling findfreeblock\n");
	uint64_t blockNumber = findFreeBlock();
	if (blockNumber == -1) {
		return -1; //failure to write block. couldnt find free block
	}
	printf("New block number from findfreeblock: %ld\n", blockNumber);
	int fd = open(diskManagerFileName, O_RDWR);
	uint64_t offset = (blockNumber) * diskManagerBlockSize;
	assert(pwrite(fd, buf, diskManagerBlockSize, offset)== diskManagerBlockSize);
	assert(close(fd) == 0);
	printf("Completed writing to disk\n");
	return blockNumber;
}

int findFreeBlock() {

	printf("Inside Disk Manager findFreeBlock\n");
	uint64_t blockNumber;
	if (currentDiskBlockNumber >= (totalBlockCount - 1)) {
		printf("current block number has exceeded disk space.. Fetching blocks from free list\n");
		uint64_t freeListBlockNumber = getBlockFromFreeList();
		if (freeListBlockNumber == -1) {
			printf("No free blocks on disk\n");
			return -1;  //No space in file to allocate
		}

		blockNumber = freeListBlockNumber;
	} else {
		printf("current block number: %d\n", currentDiskBlockNumber);
		blockNumber = currentDiskBlockNumber;
		currentDiskBlockNumber++;
	}

	printf("Free block number: %ld\n", blockNumber);
	return blockNumber;
}
