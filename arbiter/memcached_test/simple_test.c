/*
 * Compile with: gcc -lmemcached simple_test.c
 */

#include <stdio.h>
#include <string.h>
#include <libmemcached/memcached.h>
#include <unistd.h>

void main()
{
	memcached_return rc;
	memcached_st *memc;
	int i;
	
	//connecting to a server
	memcached_server_st *servers;
	memc = memcached_create(NULL);
	char servername[]= "localhost";
	servers = memcached_server_list_append(NULL, servername, 0, &rc);
	rc = memcached_server_push(memc, servers);
	memcached_server_free(servers);
	memcached_flush(memc, 0);

	//adding key/value to the server
	char *keys[]= {"fudge", "foo", "food"};
	size_t key_length[]= {5, 3, 4};
	char *values[]= {"1010", "bar", "banana"};
	size_t value_length[]= {4, 3, 6};
	
	for (i = 0; i < 3; i++) {
		rc = memcached_set(memc, keys[i], key_length[i], values[i], value_length[i], (time_t)0, (uint32_t)0);
	
		if (rc != MEMCACHED_SUCCESS)
		{
			// handle failure
			printf("ERROR: memcached_set failure!\n");
			return;
		}
		sleep(1);
	}

	//fetching values
	uint32_t flags;
	char *return_value;
	
	//char return_key[MEMCACHED_MAX_KEY];
	//size_t return_key_length;
	//size_t return_value_length;

	/* //fetching multiple values at a time
	rc = memcached_mget(memc, keys, key_length, 3);
	if (rc != MEMCACHED_SUCCESS)
	{
		// handle failure
		printf("ERROR: memcached_get failure!\n");
		return;
	}
		
	for (i = 0; i < 3; i++) {
		return_value = memcached_fetch(memc, keys[i], &key_length[i], &value_length[i], &flags, &rc);

		if (return_value != MEMCACHED_SUCCESS)
			printf("key %s: value not found\n", keys[i]);
		else
			printf("key %s: value %s\n", keys[i], return_value);
		free(return_value);
	}
	*/
	
	for (i = 0; i < 3; i++) {
		return_value = memcached_get(memc, keys[i], key_length[i], &value_length[i], &flags, &rc);

		if (rc != MEMCACHED_SUCCESS)
			printf("%s: value not found\n", keys[i]);
		else
			printf("%s: %s\n", keys[i], return_value);
		free(return_value);
		sleep(1);
	}
	
	//disconnecting from the server	
	memcached_free(memc);

}
