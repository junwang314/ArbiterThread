#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>

#include <ab_debug.h>
#include <abthread_protocol.h>
#include <ab_os_interface.h>

#include <lib/linked_list.h>

#include "arbiter.h"
#include "ipc.h"

/************************** Client State Handle ************************/

struct _client_addr_cmp {
	char *unix_addr;
	uint32_t len;	
};

static bool _cmp_client(const void *key, const void* data)
{
	struct _client_addr_cmp *cmpkey = (struct _client_addr_cmp *) key;
	struct client_desc *c = (struct client_desc *)data;

	if (!cmpkey->unix_addr || cmpkey->len != c->client_addr.addr_len)
		return false;

	if (strncmp(cmpkey->unix_addr, c->client_addr.unix_addr, cmpkey->len))
		return false;
	return true;
}


struct client_desc *arbiter_lookup_client(struct arbiter_thread *abt, 
					  char *unix_addr, 
					  uint32_t addr_len)
{
	struct _client_addr_cmp cmp;
	cmp.unix_addr = unix_addr;
	cmp.len = addr_len;

	return (struct client_desc *)linked_list_lookup(&abt->client_list, &cmp, _cmp_client);
}

/***********************************************************************/
static void handle_fork_rpc(struct arbiter_thread *abt, 
			    struct abt_request *req, 
			    struct rpc_header *hdr)
{
	struct abt_reply_header rply;
	struct abreq_fork *forkreq = (struct abreq_fork *)hdr;
	AB_INFO("Processing fork \n");

	//TODO allocate a new struct client_desc for new thread...
	// add new thread to linked list...
			
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = 0; //0 indicate success

	abt_sendreply(abt, req, &rply);

}

static void handle_malloc_rpc(struct arbiter_thread *abt, 
			      struct abt_request *req, 
			      struct rpc_header *hdr)
{
	void *ptr;
	size_t size;
	int label; //FIXME label type: uint32_t or label_t?
	struct abt_reply_header rply;
	struct abreq_malloc *mallocreq = (struct abreq_malloc *)hdr;
	AB_INFO("Processing malloc \n");

	size = mallocreq->size;
	label = mallocreq->label;

	//FIXME ablib_malloc() redesgin (in progress)
	ptr = ablib_malloc(size);
			
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = ptr;

	abt_sendreply(abt, req, &rply);

}

//this is for test purpose
static void handle_free_rpc(struct arbiter_thread *abt, 
			    struct abt_request *req, 
			    struct rpc_header *hdr)
{
	struct abt_reply_header rply;
	struct abreq_free *freereq = (struct abreq_free *)hdr;
	AB_INFO("Processing free, addr = %lx\n", (unsigned long)freereq->addr);
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
		
	abt_sendreply(abt, req, &rply);

}


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
	case ABT_FORK:
	{
		AB_INFO("arbiter: fork rpc received. req no=%d.\n", req->pkt_sn);
		handle_fork_rpc(abt, req, hdr);		
		break;
	}
	
	case ABT_MALLOC:
	{
		AB_INFO("arbiter: malloc rpc received. req no=%d.\n", req->pkt_sn);

		//the handling routine
		handle_malloc_rpc(abt, req, hdr);
		break;
	}
	case ABT_FREE:
	{
		AB_INFO("arbiter: free rpc received. req no=%d.\n", req->pkt_sn);
		handle_free_rpc(abt, req, hdr);
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

	//client table init
	init_linked_list(&abt->client_list);

	//init socket ipc
	init_arbiter_ipc(abt);
	return;
}




//global state
struct arbiter_thread arbiter;

int main()
{
	absys_thread_control(AB_SET_ME_ARBITER);
	init_arbiter_thread(&arbiter);
	server_loop(&arbiter);
	return 0;
}
