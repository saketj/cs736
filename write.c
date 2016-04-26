#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <linux/limits.h>

int main(int argc, char *argv[]) {
	char* filena = "/tmp/bpfs/";
	char *filename1=  (char *)malloc(1000);
	filename1[0]='\0';
	strcat(filename1,filena);
	strcat(filename1,argv[1]);
	printf("%s\n",filename1);

	int fd = open(filename1, O_RDWR);
	assert(fd>0);
	char *buf ="a";
	assert(pwrite(fd, buf, 1, 0 )==1);
	assert(pwrite(fd, buf, 1, 4096 )==1);
	assert(close(fd)==0);
	return 0;
}
