#include "diskmanager.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


int initializeDiskManager(const char *fileName, uint size, uint blockSize)
{

  diskManagerFileName = strdup(fileName);
  fileSize=size;
  totalBlockCount=fileSize/blockSize;
  

}

//TODO (Siddharth) - Assert statements everywhere to verify input, size of buffer.etc
int readBlock(const char *fileName, uint blockNumber, char *buf, uint blockSize){

int fd = xsyscall(open(fileName, O_RDWR));
unint offset = (blockNumber-1)*blockSize;
r = pread(fd, buf, blockSize, offset);
close(fd);
return 1;
	
}


int freeBlock(const char *fileName, uint blockNumber, uint blockSize){

char block[blockSize];
for(int i=0;i<blockSize;i++)
	block[i]='';

int fd = xsyscall(open(fileName, O_RDWR));
unint offset = (blockNumber-1)*blockSize;
r = pwrite(fd, block, blockSize, offset);
close(fd);

return 1;
}

int writeBlock(const char fileName, char *buf, uint blockSize){

uint blockNumber = findFreeBlock();
if(blockNumber == -1)
{
	return -1; //failure to write block. couldnt find free block
}
int fd = xsyscall(open(fileName, O_RDWR));
unint offset = (blockNumber-1)*blockSize;
r = pwrite(fd, buf, blockSize, offset);
close(fd);

return 1;
}


int findFreeBlock()
{

  if(currentDiskBlockNumber > totalBlockCount)
  {
  	return -1;
  }
  uint blockNumber = currentDiskBlockNumber;
  currentDiskBlockNumber++;
  return blockNumber;

}