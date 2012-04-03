/** PUBIC DOMAIN.  Use however the hell you want. 
 *
 * compile with:   gcc -­std=gnu99 ­-O3 ­-lmemcached mc_stress.c ­-o mc_stress
 *
 *  requires libmemcached 
 */
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <libmemcached/memcached.h>

double time_difference_ms(struct timeval previous, struct timeval next)
{
  double ms = 0;
  double prev_ms = 0, next_ms = 0;
  if ( previous.tv_sec < next.tv_sec ) 
  {
    prev_ms = previous.tv_sec * 1000;
    next_ms = next.tv_sec * 1000;
  }
  prev_ms += (double)previous.tv_usec / 1000;
  next_ms += (double)next.tv_usec / 1000;
  
  ms = next_ms - prev_ms;
  
  return ms;
}

inline long time_difference_us(struct timeval previous, struct timeval next)
{
  long us = 0;
  long prev_us = 0, next_us = 0;
  if ( previous.tv_sec < next.tv_sec ) 
  {
    prev_us = previous.tv_sec * 1000000;
    next_us = next.tv_sec * 1000000;
  }
  prev_us += previous.tv_usec;
  next_us += next.tv_usec;
  
  us = next_us - prev_us;
  
  return us;
}

struct timeval start_tv;

void start_timer() 
{
  static long start=0; 
  gettimeofday(&start_tv, NULL);
  /* reset timer */
  start = ((long)start_tv.tv_sec)*1000 + start_tv.tv_usec/1000;
}

// call with the number of iterations of whatever happened during the time
double stop_timer(long num)
{
  struct timeval stop_tv;
  gettimeofday(&stop_tv, NULL);
  double diff_ms = time_difference_ms(start_tv, stop_tv);
  if ( diff_ms > 1 )
  {
    double diff_s = diff_ms / (double)1000;
    double cmdsec = (double)num / diff_s;
    printf("\tTook %0.2fms, that is %0.0f keys/second\n", diff_ms, cmdsec);
    return diff_ms;
  }
  else
  {
    long diff_us = time_difference_us(start_tv, stop_tv);
    double diff_s = diff_us / (double)1000000;
    double cmdsec = (double)num / diff_s;
    printf("\tTook %dus, that is %0.0f keys/second\n", diff_us, cmdsec);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  if ( argc == 1 )
  {
    printf("Usage: %s [iterations] [key size] [value size]\n\n", argv[0]);
    printf("Runs benchmark with [iterations] number of keys, using random length \nof size [key size] set to a single value of random bytes of size [value size]\n\n");
    exit(1);
  }
  char *val;
  unsigned int setter = 1;
  int num_keys = atoi(argv[1]);
  int key_size = atoi(argv[2]);
  int value_size = 6 * 1024;
  if ( argc == 4 )
    value_size = atoi(argv[3]);
  unsigned long long getter;
  char **keys;
  char * randomstuff = (char *)malloc(value_size);
  printf("Num Keys: %d   Key size: %d  value size: %d\n", num_keys, key_size, value_size);
  
  memset(randomstuff, '\0', value_size);
  size_t val_len;
  memcached_st *memc;
  memcached_return rc;
  memcached_server_st *servers;
  size_t *lengths = (size_t *)malloc(num_keys * sizeof(size_t) );
  srand(time(NULL));
  for ( int i = 0 ; i < value_size - 1 ; i++ )
    randomstuff[i] = (char) (rand() % 26) + 97;
  //printf("random value: '%s'\n", randomstuff);
  printf("random key time: ");
  start_timer();
  // generate random keys
  keys = (char **)malloc( num_keys * sizeof(char **) );
  for ( int i = 0 ; i < num_keys ; i++ )
  {
    int j = 0;
    keys[i] = (char *)malloc( key_size + 1);
    for(  ; j < key_size ; j++ )
      keys[i][j] = (char) (rand() % 26) + 97;
    keys[i][j] = 0;
    lengths[i] = strlen(keys[i]);
    //printf("key: '%s'\n", keys[i]);
  }
  stop_timer(num_keys);
  val_len = strlen(randomstuff);
  //printf("str: '%s'\n", randomstuff);
  val = NULL;
  memc = memcached_create(NULL);

  servers = memcached_servers_parse("localhost");
  memcached_server_push(memc, servers);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, &setter);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, &setter);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_MD5_HASHING, &setter);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
  //memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);

  setter = 20 * 1024576;
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE, setter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE);
  printf("Socket send size: %ld\n", getter);
  getter = memcached_behavior_get(memc, MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE);
  printf("Socket recv size: %ld\n", getter);

  char *getval;
  uint32_t flags;
        // add key
  printf("SET %d keys... ", num_keys);
  start_timer();

  for ( int i = 0 ; i < num_keys ; i++ )
  {
    #if 1
    memcached_set(memc, keys[i], strlen(keys[i]), randomstuff, strlen(randomstuff), 3600, 0);
    //printf("."); fflush(stdout);
    #endif
    #if 0
    memcached_set(memc, keys[i], strlen(keys[i]), "bar", strlen("bar"), 3600, 0);
    #endif
  }
  stop_timer(num_keys);

  #if 1 
  printf("GET %d keys... ", num_keys);
  start_timer();
  for ( int i = 0 ; i < num_keys ; i++ )
  {
    getval = memcached_get(memc,  keys[i], strlen(keys[i]), &val_len, &flags, &rc);
    //printf("'%s'­>'%s'\n\n", keys[i], getval);
    free(getval);
  }
  stop_timer(num_keys);
  #endif
  
  #if 1
  printf("MGET %d keys... ", num_keys);
  char return_key[MEMCACHED_MAX_KEY];
  size_t return_keylen;
  char *return_value;
  size_t return_valuelen;
  int i = 0;
  start_timer();
  rc = memcached_mget(memc, keys, lengths, num_keys);
  for( ; rc == MEMCACHED_SUCCESS ; i++ )
  {
    return_value = memcached_fetch(memc, return_key, &return_keylen, &return_valuelen, &flags, &rc );
    return_key[return_keylen] = 0;
    //printf("key: '%s'\n", return_key);
  }
  stop_timer(i);
  #endif

  memcached_server_list_free(servers);
  memcached_free(memc);
  return 0;
}

