#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>	/* exit() */
#include <sys/wait.h>	/* waitpid() */
#include <sched.h>	/* clone() */

#include <abthread_protocol.h>
#include <ab_api.h>
#include <ab_debug.h>
#include <ab_os_interface.h>

#include "lib_client.h"

#define _GNU_SOURCE	/* clone() */

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
	
	AB_DBG("rc = %d\n", rc);
	//reply must be from abt
	assert(rc == sizeof(struct abt_reply_header));

	assert(recv_addr.sun_family == AF_UNIX);
	assert(!strncmp(recv_addr.sun_path, cli_state->abt_addr.sun_path, 108));
	
		
}

/************ Client state descriptor **************/

#define CLIENT_STATE_ADDR	0xa0000000
//struct abrpc_client_state _abclient;

static inline struct abrpc_client_state *get_state()
{
	//need lock in future?
	//return &_abclient;
	return (struct abrpc_client_state *)CLIENT_STATE_ADDR;
}

void init_client_state(label_t L, own_t O)
{
	//struct abrpc_client_state *cli_state = get_state();
	struct abrpc_client_state *cli_state;

	cli_state = (struct abrpc_client_state *)mmap((void *)CLIENT_STATE_ADDR, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	assert(cli_state != MAP_FAILED);

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
		AB_MSG("ab_fork: fork failed\n");
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
	
		if (rply.return_val) { //arbiter failed to register 
			AB_MSG("ab_fork: aribter failed to register\n");
			return -1;
		}
	}
	return pid;
}


static volatile void get_big()
{
	volatile int arr[4098];
	AB_DBG("write to %lx\n", &arr[4097]);
	arr[4097] = 1;
}


static int _pre_start_routine(void *_pre_arg)
{
	void * (*start_routine)(void *);
	void *arg;
	cat_t *L, *O;
	int rc;
	struct pre_arg *_p = _pre_arg;

	AB_DBG("_pre_start_routine begins\n");
	start_routine = _p->start_routine;
	arg = _p->arg;
	L = _p->L;
	O = _p->O;

	AB_DBG("setting up stack guard page at %lx.\n", _p->gdpage_start);
	rc = mprotect((void *)_p->gdpage_start, 4096, PROT_NONE);
	assert(!rc);

	//try trigger the fault
	//get_big();

	absys_thread_control(AB_SET_ME_SPECIAL);
	AB_DBG("set %d as special\n", getpid());
	init_client_state(L, O);
	(*start_routine)(arg);
	exit(0);
}



#define ABTHREAD_STACK_SIZE (4*4096)


int ab_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		      void * (*start_routine)(void *), void *arg,
		      label_t L, own_t O)
{
	pid_t pid;
	struct abreq_fork req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();
	struct pre_arg _pre_arg;

	unsigned long stack_high, gdpage_start;
	//two extras: one for guard page and one for alignment
	unsigned long alloc_size = ABTHREAD_STACK_SIZE + 2*4096;

	char *stack = (char *)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	AB_DBG("ab_pthread_create(): stack=(%p, %p)\n", stack, stack+alloc_size);
	assert(stack != MAP_FAILED);

	stack_high = (((unsigned long)stack + alloc_size) & 0xfffff000) - 32;
	
	//now clone a child thread
	_pre_arg.start_routine = start_routine;
	_pre_arg.arg = arg;
	_pre_arg.L = L;
	_pre_arg.O = O;
	_pre_arg.gdpage_start = (((unsigned long)stack + alloc_size) & 0xfffff000) - ABTHREAD_STACK_SIZE - 4096;

	pid = clone(_pre_start_routine, (void *)(stack_high) , CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, (void *)&_pre_arg);

	AB_DBG("ab_pthread_create(): pid = %d, stack = %lx\n", pid, stack_high);
	if (pid < 0) {
		AB_MSG("ab_pthread_create() failed! %s\n", strerror(errno));
		return -1;
	}

	sleep(1); //FIXME wait for child to join in order to update its mapping 

	*thread = pid;
	AB_DBG("ab_pthread_create(): *thread = %d\n", (int)*thread);

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
	
	return 0;
}

int ab_pthread_create_old(pthread_t *thread, const pthread_attr_t *attr,
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
		return -1;
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

		*thread = pid;
		AB_DBG("ab_pthread_create: *thread = %d\n", (int)*thread);

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
	}
	return 0;
}

int ab_pthread_join(pthread_t thread, void **value_ptr)
{
	struct abreq_pthread_join req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();
	pid_t wpid;
	int status;

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_PTHREAD_JOIN;

	req.pid = (uint32_t) thread;
	AB_DBG("ab_pthread_join: thread = %d\n", (int)thread);

	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	AB_DBG("ab_pthread_join: rply.return_val=%d\n", rply.return_val);
	if(rply.return_val == 0) {
		wpid = waitpid((pid_t) thread, &status, __WCLONE|__WALL);
		//FIXME for debug purpose, no need to be so complicated
		if(wpid <= 0) {
			return -1;
		}
		if(WIFEXITED(status)) {
			AB_DBG("child %lu exited normally.\n", 
			       (unsigned long)wpid);
			return 0;
		}
		if(WIFSIGNALED(status)) {
			AB_DBG("child %lu terminated by a signal.\n",
			       (unsigned long)wpid);
			return 0;
		}
		else {
			AB_DBG("wait pid returns with status %d\n", status);
			return 0;
		}
	}
	else {	//pid not found in the control group
		AB_MSG("ERROR: ab_pthread_join() failure!\n");
		return -1;
	}
}

pthread_t ab_pthread_self(void)
{
	return (pthread_t)getpid();
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

void *ab_realloc(void *ptr, size_t size)
{
	struct abreq_realloc req;
	struct abt_reply_header rply;
	struct abrpc_client_state *state = get_state();

	//prepare the header
	req.hdr.abt_magic = ABT_RPC_MAGIC;
	req.hdr.msg_len = sizeof(req);
	req.hdr.opcode = ABT_REALLOC;

	req.addr = (uint32_t)ptr;
	req.size = (uint32_t)size;
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));
	
	return (void *)rply.return_val;
}
