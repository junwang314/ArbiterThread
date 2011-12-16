#ifndef _ARBITER_H
#define _ARBITER_H

#include <stddef.h>
#include <lib/linked_list.h>

/* data structure that describes the run-time  */
/* state of an arbiter thread */

struct arbiter_thread {
	//unix domain socket for client connections
	int monitor_sock;

	//TODO More to add... e.g., child thread table...
	
	//list for active clients
	struct linked_list client_list;
};


//client identifier: unix domain socket addr
struct client_key {
	char unix_addr[256];
	uint32_t addr_len;
};


/* descriptor for a client */
struct client_desc {
	
	struct client_key client_addr;

	//process id
	uint32_t pid;

	//security information...
	
	
};

#endif  //_ARBITER_H
