#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>

static struct timeval start_ts, end_ts;

void start_timer()
{
    if (gettimeofday (&start_ts, NULL) != 0)
    {
        fprintf (stderr, "Unable to get time of day, exiting\n");
        exit (1);
    }
}

void end_timer(double count)
{
    double seconds, ns_per;
#ifndef BGQ
    double ghz=2.9;
#else
    double ghz=1.6;
#endif
    

    if (gettimeofday (&end_ts, NULL) != 0)
    {
        fprintf (stderr, "Unable to get time of day, exiting\n");
        exit (1);
    }
    /* Calculate run time */
    seconds = ((double)end_ts.tv_sec + ((double)end_ts.tv_usec * 1e-6)) -
        ((double)start_ts.tv_sec + ((double)start_ts.tv_usec * 1e-6));
    ns_per = (seconds * 1.0E9)/count;
   printf ("Took %lf seconds and %.0lfns or ~%.0lf cycles at %.1fGHz\n",
	   seconds, ns_per, ns_per * ghz, ghz);
/* Get start counter again so can do another end_timer (); */
    if (gettimeofday (&start_ts, NULL) != 0)
    {
        fprintf (stderr, "Unable to get time of day, exiting\n");
        exit (1);
    }
}

void time_exp (int count, double val_in)
{
        int i;
        volatile double val = val_in;
        double sum = 0;;
        printf ("---\n");
        printf ("Timing %i calls to exp(%.18f)\n", count, val);
        start_timer();
        for (i=1; i <= count; i++)
        {
            sum += exp(val);
        }
        end_timer((double)count);
        printf ("Sum of %i calls to exp(%.13a) is %.13a\n", count, val, sum);

}

void time_pow (int count, double val_in)
{
        int i;
        volatile double val = val_in;
        double sum = 0;;
        printf ("---\n");
        printf ("Timing %i calls to pow(%.18f, %.18f)\n", count, val, 1.5);
        start_timer();
        for (i=1; i <= count; i++)
        {
            sum += pow(val, 1.5);
        }
        end_timer((double)count);
        printf ("Sum of %i calls to pow(%.13a, %.13a) is %.13a\n", count, val,
		1.5, sum);

}
int main()
{
        int count;
        volatile double near_zero = pow(2, -53), one=1.0; 

        time_exp(10000, near_zero);
        time_exp(10000000, near_zero+near_zero);
        time_exp(10000000, one);
        time_exp(10000000, .1);

	time_pow(10000, near_zero);
	time_pow(10000, near_zero+near_zero);

	double near_one = 0x1.fffffffffffffp-1;
	near_zero=0x1.fffffffffffffp-1 - 0x1.ffffffffffffep-1;
	time_pow(10000, one);
	time_pow(10000, one-near_zero);
	
	for(count=1000;count>0;count--){
	  time_pow(10000, near_one);
	  near_one-=near_zero;
	}

#ifdef NOT_DEFINED	
	time_pow(10000, 1.0L-near_zero);
        time_pow(1000000, 1.0L-near_zero-near_zero);
        time_pow(10000, 0.9999999999999997L);
        time_pow(10000, 0.9999999999999998L);
        time_pow(10000, 0.9999999999999999L);
        time_pow(10000, 0.9999999999999996L);
        time_pow(10000, 0.999999999999999L);
        time_pow(10000, 0.1L);
#endif	
        return (0);
}

