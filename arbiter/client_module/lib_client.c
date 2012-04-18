#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* exit() */

#include <abthread_protocol.h>
#include <ab_api.h>
#include <ab_debug.h>
#include <ab_os_interface.h>

#include "lib_client.h"

/**************** socket IPC *******************/

void init_client_ipc(struct abrpc_client_state *cli_state)
{
	int rc;
	struct sockaddr_un unix_addr;

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;

	snprintf(unix_addr.sun_path, 108, "/tmp/abt_client_%d", 
		 cli_state->pid);
	//AB_DBG("unit_addr.sun_path: %s\n", unix_addr.sun_path);
	//AB_DBG("len of unix_addr.sun_path: %d\n", sizeof(unix_addr.sun_path));

	cli_state->client_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	assert(cli_state->client_sock >= 0);

	//bind the socket
	rc = bind(cli_state->client_sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr));

	assert(rc == 0);


	//prepare the dst sockaddr
	memset(&cli_state->abt_addr, 0, sizeof(cli_state->abt_addr));
	cli_state->abt_addr.sun_family = AF_UNIX;
	snprintf(cli_state->abt_addr.sun_path, 108, "%s", MONITOR_SOCKET);
	
	AB_INFO("AB Client (pid %d): SOCKET IPC initialized.\n", cli_state->pid);
	
}


//the header and the request struct must be properly filled
//before calling this function
//the reply struct is filled once the call is complete
static void _client_rpc(struct abrpc_client_state *cli_state, 
		struct rpc_header *hdr,
		struct abt_reply_header *rply)
{
	int rc;
	
	struct sockaddr_un recv_addr;
	socklen_t recv_addr_len;

	assert(hdr->msg_len >= 0);
	rc = sendto(cli_state->client_sock, 
		    hdr, 
		    hdr->msg_len, 
		    0,
		    (const struct sockaddr *)&cli_state->abt_addr,
		    sizeof(cli_state->abt_addr));
	       

	AB_DBG("rc = %d, hdr->msg_len = %d\n", rc, hdr->msg_len);
	assert(rc == hdr->msg_len);

	for(;;) {
		recv_addr_len = sizeof(recv_addr);

		//block wait abt reply, we only get the reply struct
		rc = recvfrom(cli_state->client_sock, 
			      (void *)rply, 
			      sizeof(struct abt_reply_header),
			      0,
			      (struct sockaddr *)&recv_addr,
			      &recv_addr_len);
		
		//if recv is interrupted by signal, try again
		if (rc < 0 && errno == EINTR)
			continue;

		break;
	}
	
	//reply must be from abt
	assert(rc == sizeof(struct abt_reply_header));

	assert(recv_addr.sun_family == AF_UNIX);
	assert(!strncmp(recv_addr.sun_path, cli_state->abt_addr.sun_path, 108));
	
		
}

/************ Client state descriptor **************/

struct abrpc_client_state _abclient;

static inline struct abrpc_client_state *get_state()
{
	//need lock in future?
	return &_abclient;
}

void init_client_state(label_t L, own_t O)
{
	struct abrpc_client_state *cli_state = get_state();

	cli_state->pid = (int) getpid();
	if (L != NULL) {
		memcpy(&cli_state->label, L, sizeof(label_t));
	}
	else {
		memset(&cli_state->label, 0, sizeof(label_t));;
	}
	if (O != NULL) {
		memcpy(&cli_state->ownership, O, sizeof(own_t));
	}
	else {
		memset(&cli_state->ownership, 0, sizeof(own_t));;
	}
	//ipc init
	init_client_ipc(cli_state);
	
}

/**************** API wrappers *******************/

pid_t ab_fork(label_t L, own_t O)
{
	pid_t pid;
	struct abreq_fork req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();
		
	//now fork a child thread
	pid = fork();
	
	if (pid < 0) {
		AB_MSG("Fork failed\n");
	}
	if (pid == 0){ //child thread
		absys_thread_control(AB_SET_ME_SPECIAL);
		AB_DBG("set %d as special\n", getpid());
		init_client_state(L, O);
	}
	if (pid > 0){ //parent thread
		sleep(1); //FIXME wait for child to join in order to update its mapping
		//prepare the header
		req.hdr.abt_magic = ABT_RPC_MAGIC;
		req.hdr.msg_len = sizeof(req);
		req.hdr.opcode = ABT_FORK;

		req.label = *(uint64_t *) L;
		req.ownership = *(uint64_t *) O;
		req.pid = (uint32_t)pid;
		_client_rpc(state, &req.hdr, &rply);

		//not an malformed message
		assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
		assert(rply.msg_len == sizeof(rply));
	
		if (rply.return_val) //arbiter failed to register 
			return -1;

		//return pid;
	}
	return pid;
}

int ab_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		      void * (*start_routine)(void *), void *arg,
		      label_t L, own_t O)
{
	pid_t pid;
	struct abreq_fork req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();
		
	//now clone a child thread
	//pid = clone(start_routine, 0, CLONE_FS | CLONE_FILES | CLONE_SIGVSEM, arg);
	pid = fork();	
	if (pid < 0) {
		AB_MSG("ab_pthread_create() failed\n");
	}
	if (pid == 0){ //child thread
		absys_thread_control(AB_SET_ME_SPECIAL);
		AB_DBG("set %d as special\n", getpid());
		init_client_state(L, O);
		(*start_routine)(arg);
		exit(0);
	}
	if (pid > 0){ //parent thread 
		sleep(1); //FIXME wait for child to join in order to update its mapping 

		//prepare the header 
		req.hdr.abt_magic = ABT_RPC_MAGIC; 
		req.hdr.msg_len = sizeof(req); 
		req.hdr.opcode = ABT_FORK; 

		req.label = *(uint64_t *) L;
		req.ownership = *(uint64_t *) O;
		req.pid = (uint32_t)pid;
		_client_rpc(state, &req.hdr, &rply);

		//not an malformed message
		assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
		assert(rply.msg_len == sizeof(rply));
	
		if (rply.return_val) //arbiter failed to register 
			return -1;

		//return pid;
	}
	return pid;
}

void ab_free(void *ptr)
{
	struct abreq_free req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_FREE;

	req.addr = (uint32_t)ptr;
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));
	
	//no return value needed	
}

void *ab_malloc(size_t size, label_t L)
{
	struct abreq_malloc req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_MALLOC;

	req.size = (uint32_t) size;

	req.label = *(uint64_t *) L;
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	return (void *)rply.return_val;
}

/* create a category */
cat_t create_category(cat_type t)
{
	struct abreq_create_cat req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();
	cat_t rval;

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_CREATE_CAT;

	req.cat_type = (uint8_t) t;

	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	memcpy(&rval, &rply.return_val, sizeof(cat_t));

	return rval;
}

/* get label of itself */
void get_label(label_t L)
{
	struct abreq_label req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_GET_LABEL;

	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	memcpy(L, &rply.return_val_64, sizeof(label_t));

	return;
}		

/* get the capability set of itself */
void get_ownership(own_t O)
{
	struct abreq_ownership req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_GET_OWNERSHIP;

	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	memcpy(O, &rply.return_val_64, sizeof(own_t));

}

/* get the label of a memory object */
void get_mem_label(void *ptr, label_t L)
{
	struct abreq_mem_label req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_GET_MEM_LABEL;

	req.mem = (uint32_t) ptr;
	
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	memcpy(L, &rply.return_val_64, sizeof(label_t));

	return;
}

void *ab_calloc(size_t nmemb, size_t size, label_t L)
{
	struct abreq_calloc req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_CALLOC;

	req.nmemb = (uint32_t) nmemb;
	req.size = (uint32_t) size;

	req.label = *(uint64_t *) L;
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	return (void *)rply.return_val;
}

