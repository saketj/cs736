#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int readBlock(const char *fileName, uint blockNumber, char *buf), uint blockSize;
int freeBlock(const char *fileName, uint blockNumber, uint blockSize);
int writeBlock(const char fileName, char *buf, int blockNumber, uint blockSize);


