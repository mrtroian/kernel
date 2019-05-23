#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include "cdm_utils.h"

#define BUFF_CLEAR	_IO(1,0)
#define UNDO		_IO(2,0)
#define FREE_SPACE	_IOR(3,0,int*)


void defname(char *devpath, char *program_name)
{
	char *s_name;

	s_name = devpath;
	memset(devpath, 0, 64);
	strcpy(devpath, DEVPATH);
	while (*s_name)
		s_name++;

	while (*program_name == 46 || *program_name == 47)
		program_name++;

	while (*program_name)
		*s_name++ = *program_name++;
}

int device_read(char *devpath)
{
	int fd;
	int ret;
	char *r_buff;
	char *s_buff;

	r_buff = (char*)malloc(sizeof(char) * 1024);
	s_buff = r_buff;
	memset(s_buff, 0, 1024);
	fd = open(devpath, O_RDONLY);
	if (fd == -1 || s_buff == NULL) {
		return -1;
	}
	ret = read(fd, s_buff, 1024);
	printf("read from %s:%s\n", devpath, r_buff);
	free(r_buff);
	close(fd);

	return ret;
}

int device_read_l(char *devpath)
{
	int fd;
	char *r_buff;
	char *s_buff;

	r_buff = (char*)malloc(sizeof(char) * 1024);
	s_buff = r_buff;
	memset(s_buff, 0, 1024);
	fd = open(devpath, O_RDONLY);
	if (fd == -1 || s_buff == NULL) {
		return -1;
	}
	while(read(fd, s_buff, 1024))
		printf("read from %s:%s\n", devpath, r_buff);
	free(r_buff);
	close(fd);

	return 0;
}

int device_write(char *devpath, char *msg)
{
	int fd;
	int ret;
	int msglen;

	fd = open(devpath, O_WRONLY);
	if (fd == -1)
		return -1;

	msglen = strlen(msg);
	ret = write(fd, msg, msglen);
	close(fd);

	return ret;
}

int get_free_space(char *devpath)
{
	int ret;
	int fd;

	ret = 0;
	fd = open(devpath, O_RDWR);
	if (fd == -1)
		return -1;
	ioctl(fd, FREE_SPACE, (int*) &ret);
	close(fd);

	return ret;
}

int undo(char *devpath)
{
	int ret;
	int fd;

	ret = 0;
	fd = open(devpath, O_RDWR);
	if (fd == -1)
		return -1;
	ret = ioctl(fd, UNDO);
	close(fd);

	return ret;
}

int clear(char *devpath)
{
	int ret;
	int fd;

	fd = open(devpath, O_RDWR);
	ret = -1;
	if (fd == -1)
		return -1;
	ret = ioctl(fd, BUFF_CLEAR);
	close(fd);

	return ret;
}
