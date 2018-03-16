#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>

#include "libmb.h"

class runtime_params{
public:
  unsigned iterations;
  unsigned cases;
  unsigned num_sets;
  unsigned samplerate;
  bool nonnormals;
  bool verbose;

  runtime_params():iterations(100000),cases(20000),num_sets(5000),
		   samplerate(1000),nonnormals(false),verbose(false){}
};

class range{
public:
  unsigned begin;
  unsigned end;
  unsigned count;
  unsigned sum;
  uint64_t min,max;

  range(){}
  range(unsigned b, unsigned e, unsigned c, unsigned s):
    begin(b),end(e),count(c),sum(s){}
};

class ranges:public std::vector<range>{
public:  
  typedef std::pair<timeable::param_type *, uint64_t> aresult;
  class result_t:public std::vector<aresult>{
  public:
    void free(){
      std::for_each(begin(),end(), [](aresult &e){ delete e.first;} );
      std::vector<aresult>::clear(); }
    ~result_t(){ free();}
	
    void push_back(timeable::param_type *p, uint64_t time){
      aresult a;
      a.first=p;
      a.second=time;
      std::vector<aresult>::push_back(a);}
  };
  
  ranges(timeable &tm, parameters &numbers, runtime_params &params){
    setup_ranges(tm,numbers, params); }
  void setup_ranges(timeable &tm, parameters &numbers, runtime_params &params);
  unsigned range_sort(result_t &results, parameters &numbers,
		      unsigned samplerate);
  void clear_counts(){
    std::for_each(begin(),end(),[](range &e){e.count=0;});}

  friend std::ostream &operator<<(std::ostream &os, const ranges &r);
};

static void random_spray(timeable &function, ranges &rng,
			 runtime_params &params, parameters &numbers,
			 timeable::param_type &min, timeable::param_type &max);
static void targeted_walk(timeable &function, ranges &rng, parameters &nums,
			  runtime_params &params);

/* --- Beginning of main --- */
int main(int argc, char **argv)
{
  const char *funcname="exp";
  const char *altfname=NULL;
  const char *libmname="libm.so";
  
  runtime_params params; // Governing parameters and defaults

  bool dumpnums=false;
  const char *min_str=NULL;
  const char *max_str=NULL;
  //modes
  bool randspray=false;
  bool targeted=false;

  static struct option long_options[] = {
    {"altlibm",        required_argument, 0, 'A'},
    {"altfuncname",    required_argument, 0, 'a'},
    {"cases",          required_argument, 0, 'c'},
    {"dumpnumbers",    no_argument,       0, 'd'},
    {"function",       required_argument, 0, 'f'},
    {"iterations",     required_argument, 0, 'i'},
    {"min",           required_argument, 0, 'm'},
    {"max",           required_argument, 0, 'M'},
    {"nonnormals",     no_argument,       0, 'n'},
    {"random",         no_argument,       0, 'r'},
    {"sets",           required_argument, 0, 's'},
    {"samplerate",     required_argument, 0, 'S'},
    {"targetd",        no_argument,       0, 't'},
    {"verbose",        no_argument,       0, 'v'},
    {0,                0,                 0,  0 }
  };
  int c=0;
  int digit_optind = 0;
  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;

    c = getopt_long(argc, argv, "A:a:c:df:i:S:m:M:nrs:tv",
		    long_options, &option_index);
    switch (c) {
    case -1:
      break; // also terminates the while loop
    case 'A':
      libmname=optarg;
      break;
    case 'a':
      altfname=optarg;
      break;
    case 'i':
      sscanf(optarg,"%ld",&params.iterations);
      break;
    case 's':
      sscanf(optarg,"%ld",&params.num_sets);
      break;
    case 'm':
      min_str=optarg;
      break;
    case 'M':
      max_str=optarg;
      break;
    case 'n':
      params.nonnormals=true;
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
      params.verbose=true;
      break;
    case 'f':
      funcname=optarg;
      break;
    case 'c':
      sscanf(optarg,"%ld",&params.cases);
      break;
    case 'S':
      sscanf(optarg,"%ld",&params.samplerate);
      break;
    default:
      exit(1);
    }
  }

  timeable function(libmname, funcname, altfname);
  parameters numbers(function.num_params(), argv[optind], params.nonnormals);
  ranges rng(function, numbers, params);

  if(randspray){
    timeable::param_type *min_val;
    timeable::param_type *max_val;
    if(function.num_params()==1){
      min_val=(min_str==NULL)?
	new timeable::dbl_param(-std::numeric_limits<double>::max()):
	new timeable::dbl_param(min_str);
      max_val=(max_str==NULL)?
	new timeable::dbl_param(std::numeric_limits<double>::max()):
	new timeable::dbl_param(max_str);
    }else{
      min_val=(min_str==NULL)?
	new timeable::twodbl_param(-std::numeric_limits<double>::max(),
				   -std::numeric_limits<double>::max()):
	new timeable::twodbl_param(min_str);
      max_val=(max_str==NULL)?
	new timeable::twodbl_param(std::numeric_limits<double>::max(),
				   std::numeric_limits<double>::max()):
	new timeable::twodbl_param(max_str);
    }
	
    random_spray(function, rng, params, numbers, *min_val, *max_val);
  }
  if(targeted)
    if(function.num_params())
      targeted_walk(function, rng, numbers, params);
    else
      abort();
  if(targeted || randspray)
    // print the ranges one last time.
    rng.setup_ranges(function, numbers, params); 
  if(dumpnums) std::cout << numbers << std::endl;
  exit(0);
}

void random_spray(timeable &function, ranges &rng, runtime_params &params,
		  parameters &numbers, timeable::param_type &min,
		  timeable::param_type &max){
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  std::random_device rd;  
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  ranges::result_t results;
  
  for( int j=0;j<params.num_sets;j++){
#pragma omp parallel for
    for( unsigned i=0;i<params.cases;i++){
      timeable::param_type *p;
      double sum;
      uint64_t time=function.time_func(params.iterations, params.nonnormals,
				       gen, min, max, sum, p);
      pthread_mutex_lock(&mutex);
      results.push_back( p,time); 
      pthread_mutex_unlock(&mutex);
    }

    unsigned dumped=rng.range_sort(results, numbers, params.samplerate);
    if( dumped*100/results.size()>20){
      std::cout << "Dumped: " << dumped << '/' << results.size()
		<< " Resampling ranges" << std::endl;
      rng.setup_ranges(function,numbers, params);
    }
    results.free();
    
    std::cout << "Run: " << j << std::endl << rng;
    rng.clear_counts();
  }
}

/* This really only makes sense for single parameter functions */
void targeted_walk(timeable &function, ranges &rng, parameters &numbers,
		   runtime_params &params){
  std::vector<srch_rng> srng;
  make_srngs( numbers, srng, params.cases, params.verbose);
  
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  unsigned done=0;
#pragma omp parallel for
  for(unsigned int i=0;i<srng.size();i++){
    ranges::result_t results;
    double sum;
    double cur=srng[i].begin();
    timeable::dbl_param cur_param(cur);
    do{
      uint64_t t=function.time_func(params.iterations,cur_param, sum);
      results.push_back(new timeable::dbl_param(cur), t);
      cur=std::nexttoward(cur, std::numeric_limits<double>::max());
    } while(cur<srng[i].end());

    bool flag;
    do{
      flag=true;
      pthread_mutex_lock(&mutex);
      unsigned dumped=rng.range_sort(results, numbers, params.samplerate);
      pthread_mutex_unlock(&mutex);
      
      if( dumped*100/results.size()>20){ //20% is an arbitrary
	flag=false;
	std::cout << "Dumped: " << dumped << '/' << results.size()
		  << " Resampling ranges" << std::endl;
	pthread_mutex_lock(&mutex);
	rng.setup_ranges(function, numbers, params);
	pthread_mutex_unlock(&mutex);
      }
    }while(flag==false);
    std::cout << "Search range: ";
    pthread_mutex_lock(&m);
    std::cout << ++done;
    pthread_mutex_unlock(&m);
    std::cout << '/' << srng.size() << ' ' << std::hexfloat << srng[i].begin()
	      << '-' << srng[i].end() << std::endl;
    if(params.verbose){
      pthread_mutex_lock(&mutex);
      std::cout << rng;
      pthread_mutex_lock(&mutex);
    }
  }
}

/* ------------------------------- */

std::ostream &operator<<(std::ostream &os, const ranges &r){
  std::cout << std::endl << r.size() << " R: " << std::endl;
  for( auto it=r.begin();it!=r.end();it++)
    std::cout << it->begin << '-' << it->end << " c:" << it->count << "\tm:"
              << it->sum/it->count << " \tr:" << it->min
              << ':' << it->max << ' '
              << std::endl;
}

unsigned ranges::range_sort(result_t &results, parameters &numbers,
			    unsigned samplerate){
  unsigned dumped=0;

  for( auto it=results.begin();it!=results.end();it++){
    if(it->second==0)
      continue;
    bool flag=0;
    for( auto range=begin();range!=end();range++){
      if(it->second>=range->min && it->second<=range->max){
	range->count++;
	flag=true;
	break;
      }
    }
    if(!flag){ // didn't fit into a range
      dumped++;
      if( dumped%samplerate==0){
	numbers.push_back(it->first->clone());
	it->second=0; // this number already was added
      }
    }
  }  
  return dumped;
}

void ranges::setup_ranges(timeable &tm, parameters &numbers,
			  runtime_params &params){
  result_t results;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#pragma omp parallel for
  for( int i=0;i<numbers.size();i++){
    timeable::param_type *b=numbers[i]->clone();
    double sum;
    uint64_t time=tm.time_func(params.iterations, *b, sum);

    pthread_mutex_lock(&mutex);
    results.push_back(b,time);
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
        std::vector<range>::push_back(range(begin,i-1,count,sum));
      begin=i;
      count=1;
      sum=results[i].second;
    }
  }
  std::vector<range>::push_back(range(begin,results.size()-1,count,sum));

  for( auto it=std::vector<range>::begin();it!=std::vector<range>::end();it++){
    it->min=results[it->begin].second;
    it->max=results[it->end].second;
  }
  std::cout << std::endl << std::vector<range>::size() << " Ranges: "
	    << std::endl << *this << std::endl;
  clear_counts();
}

