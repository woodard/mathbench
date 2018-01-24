#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

const unsigned iterations=100000;
const unsigned randoms=20000;

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

  uint64_t min,max;
  range(){}
  range(unsigned b, unsigned e, unsigned c, unsigned s):begin(b),end(e),
							count(c),sum(s){}
};

int main(int argc, char **argv)
{
  std::ifstream infile(argv[1]);
  std::string line;

  std::vector< double> numbers;
  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    double a;
    sscanf(line.c_str(),"%lf",&a);
    numbers.push_back(a);
  }
  
  std::vector< std::pair<double,uint64_t> > results;
  std::vector< std::pair<double,double> > sums;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  for( auto num=numbers.begin(); num!=numbers.end();num++){
    double b=*num;
    //this overwrites the variable b with the sum
    uint64_t time=time_exp(iterations, b);

    pthread_mutex_lock(&mutex);
    results.push_back(std::pair<double,uint64_t>(*num,time));
    sums.push_back(std::pair<double,double>(*num,b));
    pthread_mutex_unlock(&mutex);
  }

  std::sort(results.begin(), results.end(),
	    [](auto &left, auto &right) { return left.second < right.second; }
	    );

  unsigned count=1;
  unsigned sum=results[0].second,begin=0;
  std::vector<struct range> ranges;
  for( unsigned i=1;i<results.size()-1;i++){ // intentionally starts at 1
    auto lowgap=results[i].second-results[i-1].second;
    auto highgap=results[i+1].second-results[i].second;
    auto partmean=sum/count/20; // 20=5% which is arbitrary but works
    if( (lowgap<=highgap && highgap<partmean) || lowgap<=partmean ) { 
      count++;
      sum+=results[i].second;
      // std::cout << results[i].second << ' ';
    } else { // closer to next value
       // std::cout << std::endl << results[i].second << ' ' << sum << '/'
       // 		<< count << '=' << sum/count << ' '
       // 		<< results[i+1].second << ' ' << std::endl;
      if(count>2)
	//if it is this small it probably isn't a plateau it is a transition
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
    it->count=0;
    it->min=results[it->begin].second;
    it->max=results[it->end].second;
  }

  std::vector< std::pair<double,uint64_t> > rand_results;
  for( int j=0;j<5000;j++){
#pragma omp parallel for
    for( unsigned i=0;i<randoms;i++){
      long int x=random();
      double a=*reinterpret_cast<double*>(&x);
      double b=a;
      uint64_t time=time_exp(iterations, a);
      pthread_mutex_lock(&mutex);
      rand_results.push_back(std::pair<double,uint64_t>(b, time));
      pthread_mutex_unlock(&mutex);
    }

    double max_input;
    uint64_t max_time;
    bool max_flag;
    for( auto it=rand_results.begin();it!=rand_results.end();it++){
      bool flag=0;
      max_input=0.0;
      max_time=0;
      max_flag=0;
      // std::cout << it->second << ' ';
      for( auto range=ranges.begin();range!=ranges.end();range++){
	if(it->second>=range->min && it->second<=range->max){
	  range->count++;
	  flag=1;
	  break;
	} 
      }
      if(!flag && it->second>max_time){
	max_time=it->second;
	max_input=it->first;
	max_flag=1;
      }
	
      if( it->second>100000)// out of range
	printf("\n%.13a %ld\n",it->first, it->second);
    }
    std::cout << "Run: " << j << std::endl;
    if(max_flag)
      printf("%.13a %ld\n",max_input,max_time);
    for( auto it=ranges.begin();it!=ranges.end();it++){
      std::cout << " r:" << results[it->begin].second << ':'
		<< results[it->end].second << " c:" << it->count << ' '
		<< double(it->count*100.0)/randoms/j << '%' 
		<< std::endl;
    }
    rand_results.clear();
  }
}

