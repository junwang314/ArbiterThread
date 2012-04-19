#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>

#include <ab_debug.h>

#include <lib/linked_list.h>

#include "client.h"
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

	if (GET_FAMILY(cmpkey->unix_addr) != GET_FAMILY(c->client_addr.unix_addr))
		return false;

	if (strncmp((GET_PATH(cmpkey->unix_addr)), GET_PATH(c->client_addr.unix_addr), cmpkey->len))
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

static bool _cmp_client_pid(const void *key, const void* data)
{
	struct client_desc *c = (struct client_desc *)data;

	if (c->pid == *(uint32_t *)key)
		return false;
	else
		return true;
}

struct client_desc *arbiter_lookup_client_pid(struct arbiter_thread *abt, 
					      uint32_t pid)
{
	return (struct client_desc *)linked_list_lookup(&abt->client_list, &pid, _cmp_client_pid);
}

void *arbiter_del_client(struct arbiter_thread *abt, struct client_desc *c){
	struct list_node *node;
	node = linked_list_locate(&abt->client_list, (void *)c);
	assert(node != NULL);
	return list_del_entry(&abt->client_list, node);
}
