#include <unistd.h>
#include <stdio.h>

#include <ab_api.h>
#include <ab_debug.h>
#include <ab_os_interface.h>

#include <lib_client.h>


extern int client_test();

int main()
{
	absys_thread_control(AB_SET_ME_SPECIAL);
	
	init_client_state((label_t){}, NULL);
	
	printf("client child process created, pid = %lu.\n",
					 (unsigned long)getpid());
	
	client_test();

}

