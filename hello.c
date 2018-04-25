#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(){

	while(1)
	{
		// printf("sleep for 1s... %d\n",getpid());
		printf(".");
		fflush(stdout);
		sleep(1);
	}

	return 0;
}
