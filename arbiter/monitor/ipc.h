#ifndef _IPC_H
#define _IPC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <abthread_protocol.h>


//an ipc packet for monitor/client communication
struct abt_request {
	//packet number
	uint32_t pkt_sn;
	//may be -1 for failure
	int32_t pkt_size;
	struct rpc_header *rheader;	//TODO: check w/ Xi: never used
	char data[RPC_DATA_LEN];	

	//more to add: client information...
	
	//client socket addr
	char client_addr[256];
	socklen_t client_addr_len;
};



void init_arbiter_ipc(struct arbiter_thread *abt);
void wait_client_call(struct arbiter_thread *abt, 
		      struct abt_request *req);

void abt_sendreply(struct arbiter_thread *abt, struct abt_request *req, struct abt_reply_header *replyhdr);


#endif //_IPC_H
