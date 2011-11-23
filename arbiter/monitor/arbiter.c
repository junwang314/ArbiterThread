#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>

#include <ab_debug.h>
#include <abthread_protocol.h>
#include <ab_os_interface.h>

#include "arbiter.h"
#include "ipc.h"



struct arbiter_thread arbiter;

static void handle_client_rpc(struct arbiter_thread *abt, 
			      struct abt_request *req)
{
	struct rpc_header *hdr;
	//sanitization
	if (req->pkt_size <= 0) {
		AB_MSG("arbiter: NULL packet or recieved failed!\n");
		return;
	}
	
	if (req->pkt_size < sizeof(struct rpc_header)) {
		AB_MSG("arbiter: incomplete packt received.\n");
		return;
	}
	
	//retrieve the header
	hdr = (struct rpc_header *)req->data;
	if (hdr->abt_magic != ABT_RPC_MAGIC || hdr->msg_len < sizeof(struct rpc_header)) {
		AB_MSG("arbiter: malformed message received.\n");
		return;
	}
	
	//TODO retrive the client information according to the
	//client socket addr

	switch(hdr->opcode) {
	case ABT_MALLOC:
	{
		AB_INFO("arbiter: malloc rpc received. req no=%d.\n", req->pkt_sn);

		//the handling routine
		//handle_malloc_rpc(req, hdr);
		break;
	}
	case ABT_FREE:
	{
		AB_INFO("arbiter: free rpc received. req no=%d.\n", req->pkt_sn);
		//handle_free_rpc(req, hdr);
		break;
	}
	//more to add

	default:
		AB_MSG("arbiter rpc: Invallid OP code!\n");

	}
}


static void server_loop(struct arbiter_thread *abt)
{
	struct abt_request abreq;
	memset(&abreq, 0, sizeof(abreq));
	for(;;) {
		wait_client_call(abt, &abreq);
		handle_client_rpc(abt, &abreq);
	}
}

static void init_arbiter_thread(struct arbiter_thread *abt)
{
	//initialize the arbiter state
	
	init_arbiter_ipc(abt);
	return;
}

int main()
{
	init_arbiter_thread(&arbiter);
	server_loop(&arbiter);
	return 0;
}
