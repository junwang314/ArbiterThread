#ifndef _ABTHREAD_PROTOCAL_H
#define _ABTHREAD_PROTOCAL_H

#include <stdint.h>
#include <sys/types.h>

//OPcode for abt and client communication
enum abt_opcode {
	ABT_MALLOC,
	ABT_FREE,
	//more to add...
};

#define ABT_RPC_MAGIC      0xABC0DE

struct rpc_header {
	uint32_t abt_magic;
	uint32_t msg_len;
	uint32_t opcode;
	//number of arguments in this rpc call
	//NOTE: we assume all args are 32 bit in length
	uint8_t num_args;	
};

//maximum data length, including the header
#define RPC_DATA_LEN               128

struct abt_reply_header {
	uint32_t abt_reply_magic;
	uint32_t msg_len;
	//32bit return value
	uint32_t return_val;
};

//socket opened by monitor
#define MONITOR_SOCKET "/tmp/abt_monitor_socket"

#endif //_ABTHREAD_PROTOCAL_H
