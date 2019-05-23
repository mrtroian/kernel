#include <stdlib.h>
#include <stdio.h>
#include "cdm_utils.h"

int main(int argc, char **argv)
{
	char devpath[64];

	if (argc < 3) {
		printf("Program needs two arguments!\nSpecify number of the device to call: [ 0, 1, 2 ]\nAnd a message to be written.\n");
		return 1;
	}
	defname(devpath, argv[1]);	
	if (device_write(devpath, argv[2]) > 0)
		printf("Written on the device %s:\n%s\n", devpath, argv[2]);
	else
		printf("Write operation failed.\n");
}