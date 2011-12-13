#include <unistd.h>

#include <ab_api.h>
#include <ab_debug.h>

#include <lib_client.h>

int main()
{
	init_client_state();
	for (;;) {
		ab_free((void *)0xdeadbeef);
		sleep(1);
	}
}
