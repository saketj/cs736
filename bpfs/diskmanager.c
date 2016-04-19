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


void initializeDiskManager(const char *fileName, uint64_t size, uint64_t blockSize)
{

  diskManagerFileName = strdup(fileName);
  fileSize=size;
  totalBlockCount=fileSize/blockSize;
  diskManagerBlockSize=blockSize;
  

}

//TODO (Siddharth) - Assert statements everywhere to verify input, size of buffer.etc
int readBlock(const char *fileName, uint64_t blockNumber, char *buf){

int fd = open(fileName, O_RDWR);
uint64_t offset = (blockNumber-1)*diskManagerBlockSize;
int r = pread(fd, buf, diskManagerBlockSize, offset);
close(fd);
return 1;
	
}


void addBlockToFreeList(uint blockNumber)
{

   struct freeListNode *temp= (struct freeListNode *)malloc(sizeof(freeList));
   temp->blockNumber=blockNumber;
   temp->next=NULL;

  if(freeList==NULL)
  {
  	 
     freeList=temp; 
  }

  temp->next=freeList;
  freeList=temp;

}


uint64_t getBlockFromFreeList()
{

  if(freeList==NULL)
  {
  	return -1;
  }

  uint64_t bNumber=freeList->blockNumber;
  struct freeListNode *temp=freeList;
  freeList=freeList->next;
  free(temp);

  return bNumber;

}

int freeBlock(const char *fileName, uint64_t blockNumber){

char block[diskManagerBlockSize];
int i;
for(i=0;i<diskManagerBlockSize;i++)
	block[i]="";

int fd = open(fileName, O_RDWR);
uint64_t offset = (blockNumber-1)*diskManagerBlockSize;
int r = pwrite(fd, block, diskManagerBlockSize, offset);
close(fd);
addBlockToFreeList(blockNumber);
return 1;
}

int writeBlock(const char fileName, char *buf){

uint64_t blockNumber = findFreeBlock();
if(blockNumber == -1)
{
	return -1; //failure to write block. couldnt find free block
}
int fd = open(fileName, O_RDWR);
uint64_t offset = (blockNumber-1)*diskManagerBlockSize;
int r = pwrite(fd, buf, diskManagerBlockSize, offset);
close(fd);

return 1;
}


int findFreeBlock()
{

  uint64_t blockNumber;
  if(currentDiskBlockNumber >= (totalBlockCount-1))
  {
  	  uint64_t freeListBlockNumber = getBlockFromFreeList();
  	  if(freeListBlockNumber == -1)
  	  {
  	  	return -1;  //No space in file to allocate
  	  }
       
      blockNumber = freeListBlockNumber;

  }

  else
  {
      blockNumber = currentDiskBlockNumber;
      currentDiskBlockNumber++;
  }

  
  return blockNumber;

}