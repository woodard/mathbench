#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <getopt.h>
#include <dlfcn.h>

#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>

#include "libmb.h"

unsigned iterations=100000;
unsigned cases=20000;
unsigned num_sets=5000;
unsigned samplerate=1000;
bool nonnormals=false;
bool verbose=false;

double (*func)(double)=&exp;

std::vector< double> numbers;

static void random_spray(std::vector<struct range> &ranges);
static void targeted_walk( const std::vector< double> &numbers,
			   std::vector<struct range> &ranges);
static unsigned range_sort(std::vector< std::pair<double,uint64_t> > &results,
			   std::vector<struct range> &ranges);
static void setup_ranges( std::vector<struct range> &ranges);

static inline void Gettimeofday(struct timeval &ts){
  if (gettimeofday (&ts, NULL) != 0) {
    fprintf (stderr, "Unable to get time of day, exiting\n");
    exit (1);
  }
}

static uint64_t time_func (int count, double val, double &sum)
{
  sum=0.0;
  struct timeval start_ts, end_ts;
  
  Gettimeofday(start_ts);
  for (int i=1; i <= count; i++) {
    sum += (*func)(val);
  }
  Gettimeofday(end_ts);
  
  /* Calculate run time */
  uint64_t useconds;
  suseconds_t usecs = end_ts.tv_usec-start_ts.tv_usec;
  time_t secs = end_ts.tv_sec-start_ts.tv_sec;
  useconds = secs*1000000+usecs;
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
  bool dumpnums=false;
  static struct option long_options[] = {
    {"altlibm",        required_argument, 0, 'A'},
    {"altfuncname",    required_argument, 0, 'a'},
    {"iterations",     required_argument, 0, 'i'},
    {"random",         no_argument,       0, 'r'},
    {"targetd",        no_argument,       0, 't'},
    {"sets",           required_argument, 0, 's'},
    {"cases",          required_argument, 0, 'c'},
    {"nonnormals",     no_argument,       0, 'n'},
    {"dumpnumbers",    no_argument,       0, 'd'},
    {"samplerate",     required_argument, 0, 'h'},
    {"function",       required_argument, 0, 'f'},
    {"verbose",        no_argument,       0, 'v'},
    {0,                0,                 0,  0 }
  };
  
  void *altlibm=NULL;
  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;

    c = getopt_long(argc, argv, "A:a:c:df:i:h:nrs:tv",
		    long_options, &option_index);
    switch (c) {
    case -1:
      break; // also terminates the while loop
    case 'A':
      altlibm=dlmopen(LM_ID_NEWLM, optarg, RTLD_LAZY);
      if( altlibm==NULL){
	std::cerr << "Loading of alternative libm implementation failed: "
		  << dlerror() << std::endl;
	exit(3);
      }
      break;
    case 'a':
      func=reinterpret_cast<double (*)(double)>(dlsym(altlibm,optarg));
      /* yeah there should be some error checking here but if this didn't
	 work then the program will crash anyway */
      break;
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
    case 'd':
      dumpnums=true;
      break;
    case 'v':
      verbose=true;
      break;
    case 'f':
      func=getFunc(optarg);
      break;
    case 'c':
      sscanf(optarg,"%ld",&cases);
      break;
    case 'h':
      sscanf(optarg,"%ld",&samplerate);
      break;
    default:
      exit(1);
    }
  }

  read_numbers(argv[optind], nonnormals, numbers);

  std::vector<struct range> ranges;
  setup_ranges( ranges);
    
  if(randspray)
    random_spray( ranges);
  if(targeted)
    targeted_walk( numbers, ranges);
  if(targeted || randspray)
    setup_ranges( ranges); // print the ranges one last time.
  if(dumpnums){
    std::cout << "---------" << std::endl << std::hexfloat;
    for( auto num=numbers.begin(); num!=numbers.end();num++)
      std::cout << *num << std::endl;
  }
}

// this test is too random and really only seemed to pick out the denormals
void random_spray(std::vector<struct range> &ranges){
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  std::vector< std::pair<double,uint64_t> > results;
  
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
      double sum;
      uint64_t time=time_func(iterations, a, sum);
      pthread_mutex_lock(&mutex);
      results.push_back(std::pair<double,uint64_t>(b, time));
      pthread_mutex_unlock(&mutex);
    }

    unsigned dumped=range_sort(results,ranges);
    if( dumped*100/results.size()>20){
      std::cout << "Dumped: " << dumped << '/' << results.size()
		<< " Resampling ranges" << std::endl;
      setup_ranges( ranges);
    }
    results.clear();

    std::cout << "Run: " << j << std::endl;
    for( auto it=ranges.begin();it!=ranges.end();it++){
      std::cout << " r:" << it->min << ':'
		<< it->max << " c:" << it->count << ' '
		<< std::defaultfloat << double(it->count*100.0)/cases/j << '%' 
		<< std::endl;
    }
  }
}

unsigned range_sort(std::vector< std::pair<double,uint64_t> > &results,
		std::vector<struct range> &ranges){
  unsigned dumped=0;

  for( auto it=results.begin();it!=results.end();it++){
    if(it->second==0)
      continue;
    bool flag=0;
    for( auto range=ranges.begin();range!=ranges.end();range++){
      if(it->second>=range->min && it->second<=range->max){
	range->count++;
	flag=true;
	break;
      } 
    }
    if(!flag){
      dumped++;
      // std::cout << std::hexfloat << it->first << ' ' << it->second
      //	<< std::endl; // debug
      if( dumped%samplerate==0 &&
	  // don't add duplicate numbers
	  std::find(numbers.begin(),numbers.end(),it->first)==numbers.end()){
	numbers.push_back(it->first);
	it->second=0;
      }
    }
  }  
  return dumped;
}

void targeted_walk( const std::vector< double> &numbers,
		    std::vector<struct range> &ranges){
  std::vector<srch_rng> srng;
  make_srngs( numbers, srng, cases, verbose);
  
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  unsigned done=0;
 #pragma omp parallel for
  for(unsigned int i=0;i<srng.size();i++){
    std::vector< std::pair<double,uint64_t> > results;
    double sum;
    double cur=srng[i].begin();
    do{
      uint64_t t=time_func(iterations,cur, sum);
      results.push_back(std::pair<double,uint64_t>(cur, t));
      cur=std::nexttoward(cur, std::numeric_limits<double>::max());
    } while(cur<srng[i].end());

    bool flag;
    do{
      flag=true;
      pthread_mutex_lock(&mutex);
      unsigned dumped=range_sort(results, ranges);
      pthread_mutex_unlock(&mutex);
      
      if( dumped*100/results.size()>20){ //20% is an arbitrary
	flag=false;
	std::cout << "Dumped: " << dumped << '/' << results.size()
		  << " Resampling ranges" << std::endl;
	pthread_mutex_lock(&mutex);
	setup_ranges( ranges);
	pthread_mutex_unlock(&mutex);
      }
    }while(flag==false);
    std::cout << "Search range: ";
    pthread_mutex_lock(&m);
    std::cout << ++done;
    pthread_mutex_unlock(&m);
    std::cout << '/' << srng.size() << ' ' << std::hexfloat << srng[i].begin()
	      << '-' << srng[i].end() << std::endl;
    if(verbose){
      pthread_mutex_lock(&mutex);
      for( auto it=ranges.begin();it!=ranges.end();it++)
	std::cout << " r:" << it->min << ':' << it->max << " c:" << it->count
		  << ' ' << std::endl;      
      pthread_mutex_lock(&mutex);
    }
  }
}

void setup_ranges( std::vector<struct range> &ranges){
  std::vector< std::pair<double,uint64_t> > results;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  ranges.clear();
#pragma omp parallel for
  for( int i=0;i<numbers.size();i++){
    double b=numbers[i];
    double sum;
    uint64_t time=time_func(iterations, b, sum);

    pthread_mutex_lock(&mutex);
    results.push_back(std::pair<double,uint64_t>(numbers[i],time));
    pthread_mutex_unlock(&mutex);
  }

  // end parallel region
  std::sort(results.begin(), results.end(),
	    [](auto &left, auto &right) { return left.second < right.second; }
	    );

  unsigned count=1;
  unsigned sum=results[0].second,begin=0;
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
    it->min=results[it->begin].second;
    it->max=results[it->end].second;
    std::cout << it->begin << '-' << it->end << " c:" << it->count << "\tm:"
	      << it->sum/it->count << " \tr:" << it->min
	      << ':' << it->max << ' '
	      << std::endl;
    it->count=0;
  }
}
