#include <unistd.h>

#include <ab_api.h>
#include <ab_debug.h>

#include <lib_client.h>


extern int client_test();

int main()
{
	
	init_client_state((label_t){}, NULL);
	
	client_test();

}

