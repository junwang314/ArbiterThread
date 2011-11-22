#ifndef _IPC_H
#define _IPC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <abthread_protocol.h>


//an ipc packet for monitor/client communication
struct abt_packet {
	//packet number
	uint32_t pkt_sn;
	uint32_t pkt_size;
	struct rpc_header *rheader;
	char data[RPC_DATA_LEN];	

	//more to add: client information...
	
	//client socket addr
	char client_addr[256];
	socklen_t client_addr_len;
};



void init_arbiter_ipc(struct arbiter_thread *abt);
void wait_client_call(struct arbiter_thread *abt, 
		      struct abt_packet *pkt);


#endif //_IPC_H
