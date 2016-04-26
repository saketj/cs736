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


void initializeDiskManager(char *fileName, uint64_t size, uint64_t blockSize)
{
  printf("Initializing Disk Manager with filename %s, size of file: %ld and block size: %ld\n",fileName,size,blockSize);
  diskManagerFileName = strdup(fileName);
  fileSize=size;
  totalBlockCount=fileSize/blockSize;
  diskManagerBlockSize=blockSize;
  printf("Completed Initializing Disk Manager\n");

}

//TODO (Siddharth) - Assert statements everywhere to verify input, size of buffer.etc
int readBlock(uint64_t blockNumber, char *buf){

printf("Inside Disk Manager readBlock for blockNumber: %ld\n",blockNumber);
int fd = open(diskManagerFileName, O_RDWR);
uint64_t offset = (blockNumber)*diskManagerBlockSize;
assert(pread(fd, buf, diskManagerBlockSize, offset)==diskManagerBlockSize);
assert(close(fd)==0);
printf("Completed reading block number %ld and contents are %s\n",blockNumber,buf);
return 1;
	
}


void addBlockToFreeList(uint blockNumber)
{
   printf("Inside addBlockToFreeList: %ld\n",blockNumber);
   struct freeListNode *temp= (struct freeListNode *)malloc(sizeof(struct freeListNode));
   temp->blockNumber=blockNumber;
   temp->next=NULL;
  if(freeList==NULL)
  {
  	 
     freeList=temp;
     return; 
  }
  temp->next=freeList;
  freeList=temp;
}


uint64_t getBlockFromFreeList(){

  if(freeList==NULL)
  {
  	return -1;
  }
  printf("Inside disk manager getBlockFromFreeList\n");
  uint64_t bNumber=freeList->blockNumber;
  struct freeListNode *temp=freeList;
  freeList=freeList->next;
  free(temp);
  printf("FreeList block number: %ld",bNumber);
  return bNumber;
}

int freeBlock(uint64_t blockNumber){

printf("Inside disk manager freeBlock with block number %ld\n",blockNumber);
char block[diskManagerBlockSize];
int i;
for(i=0;i<diskManagerBlockSize;i++)
	block[i]="";

int fd = open(diskManagerFileName, O_RDWR);
uint64_t offset = (blockNumber)*diskManagerBlockSize;
assert(pwrite(fd, block, diskManagerBlockSize, offset)==diskManagerBlockSize);
assert(close(fd)==0);
printf("Completed freeing block on disk\n");
addBlockToFreeList(blockNumber);
return 1;
}

int writeBlock(char *buf){
printf("Inside Disk Manager writeBlock\n");
printf("Calling findfreeblock\n");
uint64_t blockNumber = findFreeBlock();
if(blockNumber == -1)
{
	return -1; //failure to write block. couldnt find free block
}
printf("New block number from findfreeblock: %ld\n",blockNumber);
int fd = open(diskManagerFileName, O_RDWR);
uint64_t offset = (blockNumber)*diskManagerBlockSize;
assert(pwrite(fd, buf, diskManagerBlockSize, offset)==diskManagerBlockSize);
assert(close(fd)==0);
printf("Completed writing to disk\n");
return blockNumber;
}


int findFreeBlock(){

  printf("Inside Disk Manager findFreeBlock\n"); 
  uint64_t blockNumber;
  if(currentDiskBlockNumber >= (totalBlockCount-1))
  {
      printf("current block number has exceeded disk space.. Fetching blocks from free list\n");
  	  uint64_t freeListBlockNumber = getBlockFromFreeList();
  	  if(freeListBlockNumber == -1)
  	  {
        printf("No free blocks on disk\n");
  	  	return -1;  //No space in file to allocate
  	  }
      
      blockNumber = freeListBlockNumber;
  }
  else
  {
      printf("current block number: %d\n",currentDiskBlockNumber);
      blockNumber = currentDiskBlockNumber;
      currentDiskBlockNumber++;
  }  

  printf("Free block number: %ld\n",blockNumber); 
  return blockNumber;
}