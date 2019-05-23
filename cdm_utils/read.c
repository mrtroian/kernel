#include <stdlib.h>
#include <stdio.h>
#include "cdm_utils.h"

int main(int argc, char **argv)
{
	char devpath[64];
	int ret;

	if (argc < 2){
		printf("Program needs an argument!\nSpecify number of the device to call: [ 0, 1, 2 ]\n");
		return 1;
	}

	defname(devpath, argv[1]);
	ret = device_read(devpath);
	if (ret == -1)
		printf("%s write failed\n", devpath);
	return 0;
}