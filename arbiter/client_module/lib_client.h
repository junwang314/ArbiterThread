#ifndef _LIB_CLIENT_H
#define _LIB_CLIENT_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <ab_api.h>

#include <lib/timer.h>

//run-time state of the client
struct abrpc_client_state {
	//client process id
	int pid;
	
	//label and ownership
	label_t label;
	own_t ownership;

	//unix domain socket for connecting to arbiter
	int client_sock;

	//arbiter thread listening socket addr
	struct sockaddr_un abt_addr;

};

//tramponline parameter for clone() implemented ab_pthread_create()
struct pre_arg {
	void * (*start_routine)(void *);
	void *arg;
	cat_t *L;
	cat_t *O;
	unsigned long gdpage_start;
};

void init_client_state(label_t, own_t);

#ifdef _LIBCALL_COUNT_TIME
enum libcall_code {
	PTHREAD_CREATE,
	PTHREAD_JOIN,
	PTHREAD_SELF,
	MALLOC,
	FREE,
	CREATE_CAT,
	GET_LABEL,
	GET_OWNERSHIP,
	GET_MEM_LABEL,
	CALLOC,
	REALLOC
};
#endif


#endif //_LIB_CLIENT_H
