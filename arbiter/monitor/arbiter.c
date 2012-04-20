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
//#include "lib_client.h"

/***********************************************************************/
static void handle_create_cat_rpc(struct arbiter_thread *abt,
				  struct client_desc *c,
				  struct abt_request *req, 
				  struct rpc_header *hdr)
{
	cat_t cat;
	own_t O;
	cat_type t;
	struct abt_reply_header rply;
	struct abreq_create_cat *catreq = (struct abreq_create_cat *)hdr;
	AB_INFO("Processing create category \n");

	t = catreq->cat_type;
	cat = create_cat(t);

	//add new label to ownership
	cat = add_onwership((cat_t *)&(c->ownership), cat);
	assert(cat != 0); //FIXME run out of ownership space

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	memcpy(&rply.return_val, &cat, sizeof(cat_t));

	abt_sendreply(abt, req, &rply);

}

static void handle_fork_rpc(struct arbiter_thread *abt,
			    struct client_desc *c,
			    struct abt_request *req, 
			    struct rpc_header *hdr)
{
	label_t L1, L2;
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
		AB_MSG("Security Voilation: handle_fork_rpc\n");
		
		abt_sendreply(abt, req, &rply);
		return;
	}

	//allocate a new struct client_desc for new thread
	c_new = (struct client_desc *)malloc(sizeof(struct client_desc));
	memset(c_new, 0, sizeof(c_new));

	//fill out client_desc for the new thread...
	memcpy(	&(c_new->client_addr.unix_addr), 
		&(req->client_addr), 
		sizeof(req->client_addr));
	c_new->client_addr.addr_len = req->client_addr_len;
	
	//FIXME: to get the pid of new thread in secure way
	c_new->pid  = forkreq->pid;
	c_new->label = *(uint64_t *)L2;
	c_new->ownership = *(uint64_t *)O2;
	
	GET_FAMILY(c_new->client_addr.unix_addr) = AF_UNIX;
	snprintf(GET_PATH(c_new->client_addr.unix_addr), 108, "/tmp/abt_client_%d", c_new->pid);
	c_new->client_addr.addr_len = sizeof(GET_FAMILY(c_new->client_addr.unix_addr)) + strlen(GET_PATH(c_new->client_addr.unix_addr)) + 1;
	
	//add new thread to linked list	
	list_insert_tail(&(arbiter.client_list), (void *)c_new);
	
	AB_DBG("new thread forked: pid=%d, label=%lx, ownership=%lx\n", 
		c_new->pid, (long int)c_new->label, (long int)c_new->ownership);
	
	//set up or update page tables for existing allocated memory
	malloc_update(c_new);
	
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = 0; //0 indicate success

	abt_sendreply(abt, req, &rply);

}

static void handle_pthread_join_rpc(struct arbiter_thread *abt,
				    struct client_desc *c,
				    struct abt_request *req, 
				    struct rpc_header *hdr)
{
	label_t L1, L2;
	own_t O1, O2;
	struct client_desc *c_join;
	struct abt_reply_header rply;
	struct abreq_pthread_join *pjoinreq = (struct abreq_pthread_join *)hdr;
	AB_INFO("Processing pthread_join \n");
	
	c_join = arbiter_lookup_client_pid(abt, pjoinreq->pid);
	AB_DBG("handle_pthread_join: pid = %d\n", pjoinreq->pid);
	
	//check if the joining child belongs to the control group
	if (c_join == NULL ) {
		AB_MSG("handle_pthread_join_rpc: unknown client\n");
		rply.abt_reply_magic = ABT_RPC_MAGIC;
		rply.msg_len = sizeof(rply);
		rply.return_val = -1; //-1 indicate failure
		abt_sendreply(abt, req, &rply);
		return;
	}

	*(uint64_t *)L1 = c->label;
	*(uint64_t *)O1 = c->ownership;
	*(uint64_t *)L2 = c_join->label;
	*(uint64_t *)O2 = c_join->ownership;

	//label check
	if ( check_label(L1, O1, L2, O2) ) {
		rply.abt_reply_magic = ABT_RPC_MAGIC;
		rply.msg_len = sizeof(rply);
		rply.return_val = -1; //-1 indicate failure

	//report voilation
		AB_MSG("Security Voilation: handle_pthread_join_rpc\n");
		
		abt_sendreply(abt, req, &rply);
		return;
	}

	AB_DBG("thread calling join: &c=%p, pid=%d, label=%lx, ownership=%lx\n", 
		c, c->pid, (long int)c->label, (long int)c->ownership);
	AB_DBG("thread join: &c%p, pid=%d, label=%lx, ownership=%lx\n", 
		c_join, c_join->pid, (long int)c_join->label, (long int)c_join->ownership);
	
	//deallocate c_join
	arbiter_del_client(abt, c_join); //remove from linked list
	free(c_join);

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
		AB_MSG("VOILATION: malloc voilation!\n");
	
		abt_sendreply(abt, req, &rply);
		return;
	}
	
	ptr = (void *)ablib_malloc(pid, size, L2);

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = (uint32_t)ptr;

	abt_sendreply(abt, req, &rply);

}

static void handle_free_rpc(struct arbiter_thread *abt,
			    struct client_desc *c,
			    struct abt_request *req, 
			    struct rpc_header *hdr)
{
	void *ptr;
	pid_t pid;
	ustate unit;
	label_t L1, L2;
	own_t O1;
	struct abt_reply_header rply;
	struct abreq_free *freereq = (struct abreq_free *)hdr;
	AB_INFO("Processing free, addr = %lx\n", (unsigned long)freereq->addr);

	pid = (pid_t)(c->pid);
	*(uint64_t *)L1 = c->label;
	*(uint64_t *)O1 = c->ownership;

	ptr = (void *)(freereq->addr);
	if (ptr != NULL) {
		unit = lookup_ustate_by_mem(ptr);
		memcpy(L2, unit->unit_av->label, sizeof(label_t));
	}
	else {
		*(uint64_t *)L2 = c->label; //in order to pass label check
	}
	
	//label check	
	if ( check_label(L1, O1, L2, NULL) ) {
		rply.abt_reply_magic = ABT_RPC_MAGIC;
		rply.msg_len = sizeof(rply);

		//report voilatioin
		AB_MSG("VOILATION: malloc voilation!\n");
	
		abt_sendreply(abt, req, &rply);
		return;
	}
	
	ablib_free(pid, ptr);
	
	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
		
	abt_sendreply(abt, req, &rply);

}

static void handle_get_label_rpc(struct arbiter_thread *abt,
				 struct client_desc *c,
				 struct abt_request *req, 
				 struct rpc_header *hdr)
{
	struct abt_reply_header rply;
	//struct abreq_label *catreq = (struct abreq_label *)hdr;	// not needed
	AB_INFO("Processing get label \n");

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val_64 = c->label;

	abt_sendreply(abt, req, &rply);
}

static void handle_get_ownership_rpc(struct arbiter_thread *abt,
				     struct client_desc *c,
				     struct abt_request *req, 
				     struct rpc_header *hdr)
{
	struct abt_reply_header rply;
	//struct abreq_label *catreq = (struct abreq_label *)hdr;	// not needed
	AB_INFO("Processing get ownership \n");

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val_64 = c->ownership;

	abt_sendreply(abt, req, &rply);
}

static void handle_get_mem_label_rpc(struct arbiter_thread *abt,
				 struct client_desc *c,
				 struct abt_request *req, 
				 struct rpc_header *hdr)
{
	label_t L;
	void *ptr;
	struct abt_reply_header rply;
	struct abreq_mem_label *labelreq = (struct abreq_mem_label *)hdr;
	AB_INFO("Processing get mem label \n");

	ptr = (void *)(labelreq->mem);
	lookup_label_by_mem(ptr, L);

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val_64 = *(uint64_t *)L;

	abt_sendreply(abt, req, &rply);
}	

static void handle_calloc_rpc(struct arbiter_thread *abt,
			      struct client_desc *c,
			      struct abt_request *req, 
			      struct rpc_header *hdr)
{
	void *ptr;
	size_t size, nmemb;
	label_t L1, L2;
	pid_t pid;
	own_t O1;
	struct abt_reply_header rply;
	struct abreq_calloc *callocreq = (struct abreq_calloc *)hdr;
	AB_INFO("Processing calloc \n");

	nmemb = callocreq->nmemb;
	size = callocreq->size;
	*(uint64_t *)L2 = callocreq->label;

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
		AB_MSG("VOILATION: calloc voilation!\n");
	
		abt_sendreply(abt, req, &rply);
		return;
	}
	
	ptr = (void *)ablib_calloc(pid, nmemb, size, L2);

	rply.abt_reply_magic = ABT_RPC_MAGIC;
	rply.msg_len = sizeof(rply);
	rply.return_val = (uint32_t)ptr;

	abt_sendreply(abt, req, &rply);

}


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

	if (c == NULL ) {
		AB_MSG("arbiter: unknown client\n");
		return;
	}

	switch(hdr->opcode) {
	case ABT_CREATE_CAT:
	{
		AB_INFO("arbiter: create_cat rpc received. req no=%d.\n", req->pkt_sn);
		handle_create_cat_rpc(abt, c, req, hdr);		
		break;
	}
	case ABT_FORK:
	{
		AB_INFO("arbiter: fork rpc received. req no=%d.\n", req->pkt_sn);
		handle_fork_rpc(abt, c, req, hdr);		
		break;
	}
	case ABT_PTHREAD_JOIN:
	{
		AB_INFO("arbiter: pthread_join rpc received. req no=%d.\n", req->pkt_sn);
		handle_pthread_join_rpc(abt, c, req, hdr);		
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
	case ABT_GET_LABEL:
	{
		AB_INFO("arbiter: get_label rpc received. req no=%d.\n", req->pkt_sn);
		handle_get_label_rpc(abt, c, req, hdr);
		break;
	}
	case ABT_GET_OWNERSHIP:
	{
		AB_INFO("arbiter: get_ownership rpc received. req no=%d.\n", req->pkt_sn);
		handle_get_ownership_rpc(abt, c, req, hdr);
		break;
	}	
	case ABT_GET_MEM_LABEL:
	{
		AB_INFO("arbiter: get_mem_label rpc received. req no=%d.\n", req->pkt_sn);
		handle_get_mem_label_rpc(abt, c, req, hdr);
		break;
	}
	case ABT_CALLOC:
	{
		AB_INFO("arbiter: calloc rpc received. req no=%d.\n", req->pkt_sn);

		//the handling routine
		handle_calloc_rpc(abt, c, req, hdr);
		break;
	}

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

//declaration of test function in client_test.c
//int client_test(void);

//init metadata (i.e. struct client_desc) for the first child @ arbiter side
void init_first_child(pid)
{
	struct client_desc *c_new;
		
	//allocate a new struct client_desc for new thread
	c_new = (struct client_desc *)malloc(sizeof(struct client_desc));
	memset(c_new, 0, sizeof(c_new));

	//fill out client_desc for the new thread...
	GET_FAMILY(c_new->client_addr.unix_addr) = AF_UNIX;
	snprintf(GET_PATH(c_new->client_addr.unix_addr), 108, "/tmp/abt_client_%d", pid);
	c_new->client_addr.addr_len = sizeof(GET_FAMILY(c_new->client_addr.unix_addr)) 
				+ strlen(GET_PATH(c_new->client_addr.unix_addr)) + 1;
	c_new->pid = pid;
		
	//add new thread to linked list	
	list_insert_tail(&(arbiter.client_list), (void *)c_new);
}


#define APP_EXECUTABLE "../application"


int main()
{
	pid_t pid;
       	int rc;
       	void *addr = (void *)0x80000000;

	pid = fork();
	assert(pid >= 0);
	if (pid == 0){ //first child
	
		sleep(2);
		
		//launch the application, currently we do not care about command line args
		rc = execv(APP_EXECUTABLE, NULL);
		perror("app launch failed.\n");
		assert(rc);
	}

	if (pid > 0){ //arbiter
		absys_thread_control(AB_SET_ME_ARBITER);
		init_arbiter_thread(&arbiter);
		AB_DBG("child pid: %d\n", pid);
		//init client metadata @ arbiter side
		init_first_child(pid);
		//server routine
		server_loop(&arbiter);
		return 0;
	}
}
