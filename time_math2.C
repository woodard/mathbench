#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>

#include <fstream>
#include <sstream>
#include <string>

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
int main(int argc, char **argv)
{
  std::ifstream infile(argv[1]);
  std::string line;
  while (std::getline(infile, line)){
    std::istringstream iss(line);
    double a;
    if (iss.peek()=='#')
      continue;
    if (!(iss >> a)) { break; } // error
    time_exp(100000, a);
    
  }
}

