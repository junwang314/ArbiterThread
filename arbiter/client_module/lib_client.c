#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include <abthread_protocol.h>
#include <ab_api.h>

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

void init_client_state(struct abrpc_client_state *cli_state)
{
	//TODO init other state
	cli_state->pid = (int) getpid();

	//ipc init
	init_client_ipc(cli_state);
	
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
	       


	assert(rc == hdr->msg_len);

	for(;;) {
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

/**************** API wrappers *******************/

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

//To Jun: could the label type be shrink to 32 bit?
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

	//FIXME:
	req.label = *(uint32_t *) &L;
	_client_rpc(state, &req.hdr, &rply);

	//not an malformed message
	assert(rply.abt_reply_magic == ABT_RPC_MAGIC);
	assert(rply.msg_len == sizeof(rply));

	return (void *)rply.return_val;
}


