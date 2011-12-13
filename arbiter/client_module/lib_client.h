#ifndef _LIB_CLIENT_H
#define _LIB_CLIENT_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

//run-time state of the client
struct abrpc_client_state {
	//client process id
	int pid;

	//unix domain socket for connecting to arbiter
	int client_sock;

	//arbiter thread listening socket addr
	struct sockaddr_un abt_addr;

};


void init_client_state();



#endif //_LIB_CLIENT_H
