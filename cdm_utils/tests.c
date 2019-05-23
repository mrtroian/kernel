#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include "cdm_utils.h"

#define CDM_0 "/dev/cdm.0"
#define CDM_1 "/dev/cdm.1"
#define CDM_2 "/dev/cdm.2"

int test4(void)
{
	char *msg1 = "0xFFFFFFFFFFFFFF";
	char *msg2 = "0xAAAAAAAAAAAAAA";
	char *msg3 = "0x00000000000000";

	printf("\n*********Test 4*********\n");
	printf("Writing string '%s' on %s\n", msg1, CDM_0);
	device_write(CDM_0, msg1);
	printf("Writing string '%s' on %s\n", msg2, CDM_1);
	device_write(CDM_1, msg2);
	printf("Writing string '%s' on %s\n", msg3, CDM_2);
	device_write(CDM_2, msg3);

	device_read(CDM_0);
	device_read(CDM_1);
	device_read(CDM_2);

	clear(CDM_0);
	clear(CDM_1);
	clear(CDM_2);

	return 0;
}

int test3(char *device)
{
	char *msg1 = "Hello World";
	char *msg2 = "Hellorld";
	int ret = 0;

	printf("\n*********Test 3 on %s*********\n", device);
	printf("This test should demonstrate using of IOCTL methods.\n");
	device_write(device, msg1);
	printf("'Hello World' is written to the device\n");
	device_read(device);
	device_write(device, msg2);
	device_read(device);
	printf("misspelled 'Hello World' is also written to the device\n");
	printf("Calling undo\n");
	undo(device);
	device_read(device);
	printf("Check the free space in buffer\n");
	ret = get_free_space(device);
	printf("Device %s has %d bytes of free space\n", device, ret);
	printf("Writing another string\n");
	device_write(device, "0xFF00AF0110");
	device_read(device);
	ret = get_free_space(device);
	printf("Device %s has %d bytes of free space\n", device, ret);
	printf("Clearing the buffer\n");
	clear(device);
	ret = get_free_space(device);
	printf("Device %s has %d bytes of free space\n", device, ret);
	clear(device);

	return 0;
}

int test2(char *device)
{
	int i = 129;
	char *message = "0xFF00, ";

	printf("\n*********Test 2 on %s*********\n", device);
	printf("This test should demonstrate blocking on full buffer.\n");
	printf("Program will write 8 bytes in loop until buffer has space.\n");
	printf("In 10 seconds buffer will be read and cleaned from another process.\n");

	while(i--) {
		device_write(device, message);
	}
	clear(device);
	printf("Buffer cleared\n");
	return 0;
}

int test2_fork(char *device)
{
	sleep(10);
	device_read(device);
	clear(device);
	exit(0);
}

int test1_read(char *device)
{
	int ret;

	printf("\n*********Test 1 on %s*********\n", device);
	printf("Reading from the device %s\n", device);
	printf("This test should demonstrate blocking on empty buffer.\n");
	printf("Device should block until buffer is not empty or timeout is end.\n");
	printf("In 5 seconds a string should be written from other process.\n\n");
	ret = device_read(CDM_0);
	printf("Read %d bytes from %s\n", ret, device);
	clear(device);

	return ret;
}

int test1_fork(char *device)
{
	sleep(5);
	printf("Writing to the device %s\n", device);
	device_write(device, "\nThis message is written from forked process.\n");
	exit(0);
}

int test0(char *device)
{
	int retw;
	int retr;

	retw = device_write(device, "Hello World\n");

	if (retw == -1) {
		printf("Writing failed\n");
		return -1;
	}
	printf("*********Test 0 on %s*********\n", device);
	printf("This is a simple test to check reading and writing to the device\n");
	printf("Writing succeed\n");
	retr = device_read(device);

	if (retr == -1) {
		printf("Reading failed\n");
		return -1;
	}
	printf("Reading succeed\n");
	printf("Written: %d.\n", retw);
	printf("Read: %d.\n", retr);
	clear(device);

	if (retw == retr) {
		printf("Test 0 on %s succeed\n", device);
		return 0;
	} else {
		printf("Test 0 on %s failed\n", device);
		return -1;
	}

}

int main(int argc, char **argv)
{
	if (test0(CDM_0) == -1)
		printf("Test 0 on cdm.0 failed!\n");

	if (fork()) {
		test1_read(CDM_0);
		sleep(2);
	} else {
		test1_fork(CDM_0);
	}
	if (fork()) {
		test2(CDM_0);
		sleep(2);
	} else {
		test2_fork(CDM_0);
	}
	test3(CDM_0);
	test4();
	return 0;
}