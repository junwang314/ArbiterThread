#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <string.h>

#include <abthread_protocol.h>
#include <ab_debug.h>

#include "arbiter.h"
#include "ipc.h"

void init_arbiter_ipc(struct arbiter_thread *abt)
{
	int rc;
	struct sockaddr_un unix_addr;

	//addr for the unix socket
	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	//at most of len 108, see un.h
	snprintf(unix_addr.sun_path, 108, "%s", MONITOR_SOCKET);

	//remove the pipe if exists
	unlink(unix_addr.sun_path);

	//unix domain socket using datagram
	//guaranteed deliver and in-order
	abt->monitor_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	assert(abt->monitor_sock >= 0);

	//bind the socket
	rc = bind(abt->monitor_sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr));

	assert(rc == 0);

	AB_INFO("Arbiter thread: SOCKET IPC initialized.\n");
}

//block waiting for some client rpc
//record the packet and client state
void wait_client_call(struct arbiter_thread *abt, 
		      struct abt_request *req)
{
	static unsigned long serial = 0;
	
	req->client_addr_len = sizeof(req->client_addr);

	//blocking wait
	//store the client socket addr in order to 
	//identify the client
	req->pkt_size = recvfrom(abt->monitor_sock, 
				 (void *)req->data, 
				 RPC_DATA_LEN,
				 0,
				 (struct sockaddr *)req->client_addr,
				 &req->client_addr_len);
	//AB_DBG("req->client_addr: %s\n", GET_PATH(req->client_addr));
	//AB_DBG("req->client_addr_len: %d\n", req->client_addr_len);
	//AB_DBG("req->pk_size: %d\n", req->pkt_size);
	
	req->pkt_sn = serial++;
}

//send the reply
//currently for rpcs, only the header is required
//the caller is responsible for releasing the header
void abt_sendreply(struct arbiter_thread *abt, struct abt_request *req, struct abt_reply_header *replyhdr)
{
	int rc;
	rc = sendto(abt->monitor_sock, 
		    (void *)replyhdr, 
		    sizeof(struct abt_reply_header),
		    0,
		    (struct sockaddr *)req->client_addr,
		    req->client_addr_len);
	assert(rc >=0);	
}
