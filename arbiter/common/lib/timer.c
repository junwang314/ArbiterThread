#include <stdio.h>
#include <sys/time.h> /*gettimeofday()*/

#include <lib/timer.h>

double time_difference_ms(struct timeval previous, struct timeval next)
{
	double ms = 0;
	double prev_ms = 0, next_ms = 0;
	if ( previous.tv_sec < next.tv_sec ) {
		prev_ms = previous.tv_sec * 1000;
		next_ms = next.tv_sec * 1000;
	}
	prev_ms += (double)previous.tv_usec / 1000;
	next_ms += (double)next.tv_usec / 1000;
	
	ms = next_ms - prev_ms;
	
	return ms;
}

long time_difference_us(struct timeval previous, struct timeval next)
{
	long us = 0;
	long prev_us = 0, next_us = 0;
	if ( previous.tv_sec < next.tv_sec ) {
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
	static long start = 0; 
	gettimeofday(&start_tv, NULL);
	/* reset timer */
	start = ((long)start_tv.tv_sec)*1000 + start_tv.tv_usec/1000;
}

// call with the number of iterations of whatever happened during the time
double stop_timer(long num, FILE *fp)
{
	struct timeval stop_tv;
	gettimeofday(&stop_tv, NULL);
	double diff_ms = time_difference_ms(start_tv, stop_tv);
	if ( diff_ms > 1 ) {
		double diff_s = diff_ms / (double)1000;
		double cmdsec = (double)num / diff_s;
		printf("\tTook %0.2fms, that is %0.0f keys/second\n", diff_ms, cmdsec);
		if(fp != NULL)
			fprintf(fp, "\tTook %0.2fms, that is %0.0f keys/second\n", diff_ms, cmdsec);
		return diff_ms;
	}
	else {
		long diff_us = time_difference_us(start_tv, stop_tv);
		double diff_s = diff_us / (double)1000000;
		double cmdsec = (double)num / diff_s;
		printf("\tTook %dus, that is %0.0f keys/second\n", diff_us, cmdsec);
		if(fp != NULL)
			fprintf(fp, "\tTook %dus, that is %0.0f keys/second\n", diff_us, cmdsec);
	}
	return 0;
}
