#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

inline void Gettimeofday(struct timeval &ts){
  if (gettimeofday (&ts, NULL) != 0) {
    fprintf (stderr, "Unable to get time of day, exiting\n");
    exit (1);
  }
}

double time_exp (int count, double val_in)
{
  volatile double val = val_in;
  double sum = 0;;
  struct timeval start_ts, end_ts;
  
  Gettimeofday(start_ts);
  for (int i=1; i <= count; i++) {
    sum += exp(val);
  }
  Gettimeofday(end_ts);
  
  /* Calculate run time */
  double seconds, ns_per;
  suseconds_t usecs = end_ts.tv_usec-start_ts.tv_usec;
  time_t secs = end_ts.tv_sec-start_ts.tv_sec;
  seconds = secs+usecs*1e-6;
  ns_per = (seconds * 1.0E9)/count;

  return ns_per;
}

int main(int argc, char **argv)
{
  std::ifstream infile(argv[1]);
  std::string line;
  std::vector< std::pair<double, double> > results;
  while (std::getline(infile, line)){
    std::istringstream iss(line);
    double a;
    if (iss.peek()=='#')
      continue;
    if (!(iss >> a)) { break; } // error
    results.push_back(std::pair<double,double>(a, time_exp(100000, a)));
  }

  for( std::vector< std::pair<double,double> >::iterator i=results.begin();
       i!=results.end();
       i++)
    std::cout << i->first << ' ' << i->second << std::endl;    

}

