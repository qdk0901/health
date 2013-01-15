#include <unistd.h>
#include "common.h"

int main(int argc, char *argv[])
{
	if (init_workers() != STATUS_OK)
		return STATUS_FAILED;
	if (init_sampler() != STATUS_OK)
		return STATUS_FAILED;
	if (init_server() != STATUS_OK)
		return STATUS_FAILED;
	
	for(;;) {
		sleep(0x00ffffff);	
	}
	return 0;
}
