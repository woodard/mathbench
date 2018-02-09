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

static uint64_t time_exp (int count, double &val)
{
  double sum = 0.0;
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
  bool dumpnums=false;
  
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
      {"dumpnumbers",    no_argument,       0, 'd'},
      {"samplerate",     required_argument, 0, 'h'},
      {"function",       required_argument, 0, 'f'},
      {"verbose",        no_argument,       0, 'v'},
      {0,                0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "c:df:i:h:nrs:tv",
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
    case 'd':
      dumpnums=true;
      break;
    case 'v':
      verbose=true;
      break;
    case 'f':{
      std::string s(optarg);
      if(s=="tan")
	func=&tan;
      else if(s=="cos")
	func=&cos;
      else if(s=="sin")
	func=&sin;
      else if(s=="acosh")
	func=&acosh;
      else if(s=="acos")
	func=&acos;
      else if(s=="asinh")
	func=&asinh;
      else if(s=="asin")
	func=&asin;
      else if(s=="atanh")
	func=&atanh;
      else if(s=="cosh")
	func=&cosh;
      else if(s=="cos")
	func=&cos;
      else if(s=="exp2")
	func=&exp2;
      else if(s=="log2")
	func=&log2;
      else if(s=="log")
	func=&log;
      else if(s=="rint")
	func=&rint;
      else if(s=="sinh")
	func=&sinh;
      else if(s=="sqrt")
	func=&sqrt;
      else if(s=="tanh")
	func=&tanh;
      else if(s=="trunc")
	func=&trunc;
      else
	exit(2);
      break;}
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
  
  std::ifstream infile(argv[optind]);
  // TODO: some error handling here
  std::string line;

  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    double a;
    sscanf(line.c_str(),"%lf",&a);
    if(isnormal(a) || nonnormals==true)
      numbers.push_back(a);
  }

  std::vector<struct range> ranges;
  setup_ranges( ranges);
    
  if(randspray)
    random_spray( ranges);
  if(targeted)
    targeted_walk( numbers, ranges);
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
      uint64_t time=time_exp(iterations, a);
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
      if( dumped%samplerate==0){
	numbers.push_back(it->first);
	it->second=0;
      }
    }
  }  
  return dumped;
}

void targeted_walk( const std::vector< double> &numbers,
		    std::vector<struct range> &ranges){

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

  unsigned idx=0;
  /* numbers will get added to the list to expand the size of the ranges
     we don't want to continue the test beyond the initial range of numbers
     otherwise the test goes on too long */
  unsigned orig_numbers=numbers.size();
  for( auto num=numbers.begin();num!=numbers.end();num++,idx++){
    db orig;
    if( idx>orig_numbers)
      break;
    std::vector< std::pair<double,uint64_t> > results;
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
      
    bool flag;
    do{
      flag=true;
      unsigned dumped=range_sort(results, ranges);
      if( dumped*100/results.size()>20){ //20% is an arbitrary
	flag=false;
	std::cout << "Dumped: " << dumped << '/' << results.size()
		  << " Resampling ranges" << std::endl;
	setup_ranges( ranges);
      }
    }while(flag==false);
    std::cout << idx << '/' << orig_numbers << '(' << numbers.size() << ')'
	      << " Around: " << std::hexfloat << *num << ':' << std::endl;
    if(verbose)
      for( auto it=ranges.begin();it!=ranges.end();it++){
	std::cout << " r:" << it->min << ':'
		  << it->max << " c:" << it->count << ' '
		  << std::endl;
    }
    results.clear();
  }
}

void setup_ranges( std::vector<struct range> &ranges){
  std::vector< std::pair<double,uint64_t> > results;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  ranges.clear();
#pragma omp parallel for
  for( int i=0;i<numbers.size();i++){
    double b=numbers[i];
    //this overwrites the variable b with the sum
    uint64_t time=time_exp(iterations, b);

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
