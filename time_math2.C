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
#include <algorithm>

inline void Gettimeofday(struct timeval &ts){
  if (gettimeofday (&ts, NULL) != 0) {
    fprintf (stderr, "Unable to get time of day, exiting\n");
    exit (1);
  }
}

uint64_t time_exp (int count, double &val)
{
  double sum = 0.0;
  struct timeval start_ts, end_ts;
  
  Gettimeofday(start_ts);
  for (int i=1; i <= count; i++) {
    sum += exp(val);
  }
  Gettimeofday(end_ts);
  
  /* Calculate run time */
  uint64_t useconds;
  suseconds_t usecs = end_ts.tv_usec-start_ts.tv_usec;
  time_t secs = end_ts.tv_sec-start_ts.tv_sec;
  useconds = secs*1000000+usecs;
  val=sum;
  return useconds;
}

struct range{
  unsigned begin;
  unsigned end;
  unsigned count;
  unsigned sum;
  range(){}
  range(unsigned b, unsigned e, unsigned c, unsigned s):begin(b),end(e),
							count(c),sum(s){}
};

int main(int argc, char **argv)
{
  unsigned count=100000;
  
  std::ifstream infile(argv[1]);
  std::string line;
  std::vector< std::pair<double,uint64_t> > results;
  std::vector< std::pair<double,double> > sums;
  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    double a;
    sscanf(line.c_str(),"%lf",&a);
    //  printf("%.13a\n",a);

    double b=a;
    //this overwrites the variable a with the sum
    results.push_back(std::pair<double,uint64_t>(b, time_exp(count, a)));
    sums.push_back(std::pair<double,double>(b,a));
  }

  std::sort(results.begin(), results.end(),
	    [](auto &left, auto &right) { return left.second < right.second; }
	    );

  count=1;
  unsigned sum=results[0].second,begin=0;
  std::vector<struct range> ranges;
  for( unsigned i=1;i<results.size()-1;i++){ // intentionally starts at 1
    auto lowgap=results[i].second-results[i-1].second;
    auto highgap=results[i+1].second-results[i].second;
    auto partmean=sum/count/20; // 20=5% which is arbitrary but works
    if( (lowgap<=highgap && highgap<partmean) || lowgap<=partmean ) { 
      count++;
      sum+=results[i].second;
      std::cout << results[i].second << ' ';
    } else { // closer to next value
      std::cout << std::endl << results[i].second << ' ' << sum << '/'
		<< count << '=' << sum/count << ' '
		<< results[i+1].second << ' ' << std::endl;
      if(count>2)
	ranges.push_back(range(begin,i-1,count,sum));
      begin=i;
      count=1;
      sum=results[i].second;
    }
  }
  ranges.push_back(range(begin,results.size()-1,count,sum));
  
  std::cout << std::endl << ranges.size() << " Ranges: " << std::endl;
  for( auto it=ranges.begin();it!=ranges.end();it++){
    std::cout << it->begin << '-' << it->end << " c:" << it->count << " m:"
	      << it->sum/it->count << " r:" << results[it->begin].second << ':'
	      << results[it->end].second << ' '
	      << double(results[it->end].second-results[it->begin].second)/it->count
	      << std::endl;
  }
}

