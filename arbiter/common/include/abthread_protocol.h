#ifndef _ABTHREAD_PROTOCAL_H
#define _ABTHREAD_PROTOCAL_H

#include <stdint.h>
#include <sys/types.h>

#include <ab_api.h>

//OPcode for abt and client communication
enum abt_opcode {
	ABT_FORK,
	ABT_MALLOC,
	ABT_FREE,
	ABT_CREATE_CAT
	//more to add...
};

#define ABT_RPC_MAGIC      0xABC0DE

struct rpc_header {
	uint32_t abt_magic;
	uint32_t msg_len;
	uint32_t opcode;
};

//maximum data length, including the header
#define RPC_DATA_LEN               256

struct abt_reply_header {
	uint32_t abt_reply_magic;
	uint32_t msg_len;
	//32bit return value
	uint32_t return_val;
};

//socket opened by monitor
#define MONITOR_SOCKET "/tmp/abt_monitor_socket"

//AbThread API requests
struct abreq_create_cat {
	struct rpc_header hdr;
	uint8_t cat_type;
};

struct abreq_fork{
	struct rpc_header hdr;
	uint64_t label;
	uint64_t ownership;
	uint32_t pid;
};

struct abreq_malloc {
	struct rpc_header hdr;
	uint32_t size;
	uint64_t label;
};

struct abreq_free {
	struct rpc_header hdr;
	uint32_t addr;	
};

struct abreq_create_category {
	struct rpc_header hdr;
	uint64_t label;
};





#endif //_ABTHREAD_PROTOCAL_H
