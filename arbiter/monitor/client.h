#ifndef _CLIENT_H
#define _CLIENT_H

#include <stdint.h>
#include <sys/types.h>
#include <ab_api.h>
#include <lib/linked_list.h>

#include "arbiter.h"


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

	//security information: label and ownership
	uint64_t label;
	uint64_t ownership;
	
};

//client manipulation
struct client_desc *arbiter_lookup_client(struct arbiter_thread *abt, 
					  char *unix_addr, 
					  uint32_t addr_len);

#endif //_CLIENT_H
