#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <getopt.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

unsigned iterations=100000;
unsigned cases=20000;
unsigned num_sets=5000;
bool nonnormals=false;

static void random_spray(std::vector<struct range> &ranges);
static void targeted_walk( const std::vector< double> &numbers,
			   std::vector<struct range> &ranges);
static void range_sort(std::vector< std::pair<double,uint64_t> > &results,
		       std::vector<struct range> &ranges);
  
static inline void Gettimeofday(struct timeval &ts){
  if (gettimeofday (&ts, NULL) != 0) {
    fprintf (stderr, "Unable to get time of day, exiting\n");
    exit (1);
  }
}

static uint64_t time_exp (int count, double &val)
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
  int c=0;
  int digit_optind = 0;
  bool randspray=false;
  bool targeted=false;
  
  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"iterations",     required_argument, 0, 'i'},
      {"random",         no_argument,       0, 'r'},
      {"targetd",        no_argument,       0, 't'},
      {"sets",           required_argument, 0, 's'},
      {"cases",          required_argument, 0, 'c'},
      {"nonnormals",     no_argument,       0, 'n'},
      {0,                0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "icnrst",
		    long_options, &option_index);
    switch (c) {
    case -1:
      break; // also terminates the while loop
    case 'i':
      sscanf(optarg,"%ld",&iterations);
      break;
    case 's':
      sscanf(optarg,"%ld",&num_sets);
      break;
    case 'n':
      nonnormals=true;
      break;
    case 'r':
      randspray=true;
      break;
    case 't':
      targeted=true;
      break;
    case 'c':
      sscanf(optarg,"%ld",&cases);
      break;
    default:
      exit(1);
    }
  }
  
  std::ifstream infile(argv[optind]);
  // TODO: some error handling here
  std::string line;

  std::vector< double> numbers;
  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    double a;
    sscanf(line.c_str(),"%lf",&a);
    if(isnormal(a) || nonnormals==true)
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
    } else { // closer to next value
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
    std::cout << it->begin << '-' << it->end << " c:" << it->count << "\tm:"
	      << it->sum/it->count << " \tr:" << results[it->begin].second
	      << ':' << results[it->end].second << ' '
	      << double(results[it->end].second-results[it->begin].second)/it->count
	      << std::endl;
    it->count=0;
    it->min=results[it->begin].second;
    it->max=results[it->end].second;
  }

  if(randspray)
    random_spray( ranges);
  if(targeted)
    targeted_walk( numbers, ranges);
}

// this test is too random and really only seemed to pick out the denormals
void random_spray(std::vector<struct range> &ranges){
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  std::vector< std::pair<double,uint64_t> > rand_results;
  
  for( int j=0;j<num_sets;j++){
#pragma omp parallel for
    for( unsigned i=0;i<cases;i++){
      long int x=random();
      double a=*reinterpret_cast<double*>(&x);
      if(nonnormals==false){
	/* for some reason !isnormal(a) and then running random(x)
	   didn't work here and I ended up in a seemingly infinite
	   loop as if isnormal() wasn't working. This should pop the
	   subnormals out.  Yes it biases the numbers toward the low
	   normals but that may be a good thing */
	while( !isnormal(a))
	  a*=2;
      }
      
      double b=a;
      uint64_t time=time_exp(iterations, a);
      pthread_mutex_lock(&mutex);
      rand_results.push_back(std::pair<double,uint64_t>(b, time));
      pthread_mutex_unlock(&mutex);
    }

    range_sort(rand_results,ranges);
    rand_results.clear();

    std::cout << "Run: " << j << std::endl;
    for( auto it=ranges.begin();it!=ranges.end();it++){
      std::cout << " r:" << it->min << ':'
		<< it->max << " c:" << it->count << ' '
		<< std::defaultfloat << double(it->count*100.0)/cases/j << '%' 
		<< std::endl;
    }
  }
}

void range_sort(std::vector< std::pair<double,uint64_t> > &results,
		std::vector<struct range> &ranges){
  double max_input,min_input;
  uint64_t max_time,min_time;
  bool max_flag,min_flag;
  unsigned dumped=0;

  max_input=min_time=0.0;
  max_time=min_time=0;
  max_flag=min_flag=false;
  for( auto it=results.begin();it!=results.end();it++){
    bool flag=0;


    // std::cout << it->second << ' ';
    for( auto range=ranges.begin();range!=ranges.end();range++){
      if(it->second>=range->min && it->second<=range->max){
	range->count++;
	flag=true;
	break;
      } 
    }
    if(!flag){
      dumped++;
      // std::cout << std::hexfloat << it->first << ' ' << it->second << " d"
      // 		<< std::endl;
      if( it->second>max_time){
	max_time=it->second;
	max_input=it->first;
	max_flag=true;
      }
      if( !min_flag || it->second<min_time){
	min_time=it->second;
	min_input=it->second;
	min_flag=true;
      }
    }
	
    if( it->second>100000)
      std::cout << std::hexfloat << it->first << ' ' << it->second << " big"
		<< std::endl;
  }
  if(max_flag)
    std::cout << std::hexfloat << max_input << ' ' << max_time << " max"
	      << std::endl;
  if(min_flag)
    std::cout << std::hexfloat << min_input << ' ' << min_time << " min"
	      << std::endl;
  
  std::cout << "Dumped " << dumped << " values" << std::endl;
}

void targeted_walk( const std::vector< double> &numbers,
		    std::vector<struct range> &ranges){

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  std::vector< std::pair<double,uint64_t> > results;

  union db {
    double d;
    struct {
      unsigned long m : 52;
      unsigned long es : 1;
      unsigned long e : 10;
      unsigned long s : 1;
    }bf;
    unsigned char c[8];
  };

  for( auto num=numbers.begin();num!=numbers.end();num++){
    db orig;
#pragma omp parallel for
    for(int i=1;i<cases;i++){
      orig.d=*num;
      double a1,a2;
      double b1,b2;
      orig.bf.m-=i; 
      a1=b1=orig.d;
      orig.bf.m+=2*i;
      a2=b2=orig.d;
      uint64_t t1=time_exp(iterations,a1);
      uint64_t t2=time_exp(iterations,a2);
      pthread_mutex_lock(&mutex);
      results.push_back(std::pair<double,uint64_t>(b1, t1));
      results.push_back(std::pair<double,uint64_t>(b2, t2));      
      pthread_mutex_unlock(&mutex);
    }

    range_sort(results, ranges);
    std::cout << "Around: " << std::hexfloat << *num << ':' << std::endl;
    for( auto it=ranges.begin();it!=ranges.end();it++){
      std::cout << " r:" << it->min << ':'
		<< it->max << " c:" << it->count << ' '
		<< std::endl;
    }
    results.clear();
  }
}
