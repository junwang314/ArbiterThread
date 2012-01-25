#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h> /* malloc() */

#include <ab_debug.h>
#include <abthread_protocol.h>
#include <ab_os_interface.h>

#include <lib/linked_list.h>

#include "arbiter.h"
#include "ipc.h"
#include "client.h"
#include "ablib_malloc.h" /* ablib_malloc(), ablib_free() */

/***********************************************************************/
static void handle_fork_rpc(struct arbiter_thread *abt,
			    struct client_desc *c,
			    struct abt_request *req, 
			    struct rpc_header *hdr)
{
	label_t L1, L2; //FIXME label type: uint32_t or label_t?
	own_t O1, O2;
	struct client_desc *c_new;
	struct abt_reply_header rply;
	struct abreq_fork *forkreq = (struct abreq_fork *)hdr;
	AB_INFO("Processing fork \n");
	
	*(uint64_t *)L1 = c->label;
	*(uint64_t *)O1 = c->ownership;
	*(uint64_t *)L2 = forkreq->label;
	*(uint64_t *)O2 = forkreq->ownership;

	//label check
	if ( check_label(L1, O1, L2, O2) ) {
		rply.abt_reply_magic = ABT_RPC_MAGIC;
		rply.msg_len = sizeof(rply);
		rply.return_val = -1; //-1 indicate failure

	//report voilation
		
		abt_sendreply(abt, req, &rply);
		return;
	}

	//allocate a new struct client_desc for new thread
	c_new = (struct client_desc *)malloc(sizeof(struct client_desc));
	memset(c_new, 0, sizeof(c_new));

	//fill out client_desc for the new thread... TODO check with Xi
	memcpy(	&(c_new->client_addr.unix_addr), 
		&(req->client_addr), 
		sizeof(req->client_addr));  //?
	c_new->client_addr.addr_len = req->client_addr_len;  //?
	
	//FIXME: how to get the pid of new thread
	//c_new->pid ;
	c_new->label = *(uint64_t *)L2;
	c_new->ownership = *(uint64_t *)O2;
	
	//add new thread to linked list	
	list_insert_tail(&(arbiter.client_list), (void *)c_new);
	
	//TODO set up page tables for existing allocated memory
			
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = 0; //0 indicate success

	abt_sendreply(abt, req, &rply);

}

static void handle_malloc_rpc(struct arbiter_thread *abt,
			      struct client_desc *c,
			      struct abt_request *req, 
			      struct rpc_header *hdr)
{
	void *ptr;
	size_t size;
	label_t L1, L2;
	pid_t pid;
	own_t O1;
	struct abt_reply_header rply;
	struct abreq_malloc *mallocreq = (struct abreq_malloc *)hdr;
	AB_INFO("Processing malloc \n");

	size = mallocreq->size;
	*(uint64_t *)L2 = mallocreq->label;

	pid = (pid_t)(c->pid);
	*(uint64_t *)L1 = c->label;
	*(uint64_t *)O1 = c->ownership;

	//label check	
	if ( check_label(L1, O1, L2, NULL) ) {
		ptr = NULL;
		rply.abt_reply_magic = ABT_RPC_MAGIC;
		rply.msg_len = sizeof(rply);
		rply.return_val = (uint32_t)ptr;

	//report voilatioin
	
		abt_sendreply(abt, req, &rply);
		return;
	}
	

	//FIXME ablib_malloc() redesgin (in progress)
	ptr = (void *)ablib_malloc(pid, size, L2);

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = (uint32_t)ptr;

	abt_sendreply(abt, req, &rply);

}

//this is for test purpose
static void handle_free_rpc(struct arbiter_thread *abt,
			    struct client_desc *c,
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
/*
static void handle_create_category_rpc(struct arbiter_thread *abt,
				       struct client_desc *c,
				       struct abt_request *req, 
				       struct rpc_header *hdr)
{
	static cat_t cat_gen = 0;
	if (++cat_gen >= 0b10000000) {
		printf("ERROR: category used up\n");	
		return -1;
	}
	return (cat_t) (t || (cat_gen && 0b01111111));
}
*/
static void handle_client_rpc(struct arbiter_thread *abt, 
			      struct abt_request *req)
{
	struct rpc_header *hdr;
	struct client_desc *c;
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
	
	//retrive the client information according to the client socket addr
	c = arbiter_lookup_client(abt, req->client_addr, req->client_addr_len);

	if (c == NULL && hdr->opcode != ABT_FORK ) { //TODO: check with Xi
		AB_MSG("arbiter: unknown client\n");
		return;
	}

	switch(hdr->opcode) {
	case ABT_FORK:
	{
		AB_INFO("arbiter: fork rpc received. req no=%d.\n", req->pkt_sn);
		handle_fork_rpc(abt, c, req, hdr);		
		break;
	}
	
	case ABT_MALLOC:
	{
		AB_INFO("arbiter: malloc rpc received. req no=%d.\n", req->pkt_sn);

		//the handling routine
		handle_malloc_rpc(abt, c, req, hdr);
		break;
	}
	case ABT_FREE:
	{
		AB_INFO("arbiter: free rpc received. req no=%d.\n", req->pkt_sn);
		handle_free_rpc(abt, c, req, hdr);
		break;
	}
	//TODO more to add

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
