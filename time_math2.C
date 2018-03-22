#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <dlfcn.h>

#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>

#include "timeable.h"
#include "ranges.h"
#include "parameters.h"

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

static void random_spray(timeable &function, ranges &rng,
			 runtime_params &params, parameters_t &numbers,
			 param_t &min, param_t &max);
static void targeted_walk(timeable &function, ranges &rng, parameters_t &nums,
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
  parameters_t numbers(function.num_params(), argv[optind], params.nonnormals);
  parameters_t dumpees;
  ranges rng(function, numbers, params.iterations);
  std::cout << rng << std::endl;

  if(randspray){
    param_t *min_val;
    param_t *max_val;
    if(function.num_params()==1){
      min_val=(min_str==NULL)?
	new dbl_param_t(-std::numeric_limits<double>::max()):
	new dbl_param_t(min_str);
      max_val=(max_str==NULL)?
	new dbl_param_t(std::numeric_limits<double>::max()):
	new dbl_param_t(max_str);
    }else{
      min_val=(min_str==NULL)?
	new twodbl_param_t(-std::numeric_limits<double>::max(),
				   -std::numeric_limits<double>::max()):
	new twodbl_param_t(min_str);
      max_val=(max_str==NULL)?
	new twodbl_param_t(std::numeric_limits<double>::max(),
				   std::numeric_limits<double>::max()):
	new twodbl_param_t(max_str);
    }
	
    random_spray(function, rng, params, numbers, *min_val, *max_val);
  }
  if(targeted)
    if(function.num_params())
      targeted_walk(function, rng, numbers, params);
    else
      abort();
  // print the ranges one last time.
  if(targeted || randspray){
    rng.setup_ranges(function, numbers, params.iterations);
    std::cout << rng << std::endl;
  }
  if(dumpnums) std::cout << numbers << std::endl;
  exit(0);
}

void random_spray(timeable &function, ranges &rng, runtime_params &params,
		  parameters_t &numbers, param_t &min,
		  param_t &max){
  std::random_device rd;  
  std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  
#pragma omp parallel for
  for( int j=0;j<params.num_sets;j++){
    timeable::results_t results;
    std::cout << "Set " << j << '/' << params.num_sets << std::endl;
    for( unsigned i=0;i<params.cases;i++){
      param_t *p;
      double sum;
      uint64_t time=function.time_func(params.iterations, params.nonnormals,
				       gen, min, max, sum, p);
      results.push_back( p,time); 
    }

    parameters_t dumpees;
    unsigned dumped=rng.range_sort(results, dumpees);
    if( dumped*100/results.size()>20){
      std::cout << "Dumped: " << dumped << '/' << results.size()
		<< " Resampling ranges" << std::endl;
      numbers.push_back(dumpees, params.samplerate);
      rng.setup_ranges(function,numbers,params.iterations);
    }
    results.free();
    
    std::cout << "Run: " << j << std::endl << rng;
    rng.clear_counts();
  }
}

/* This really only makes sense for single parameter functions */
void targeted_walk(timeable &function, ranges &rng, parameters_t &numbers,
		   runtime_params &params){
  std::vector<srch_rng> srng;
  make_srngs( numbers, srng, params.cases, params.verbose);
  
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  unsigned done=0;
#pragma omp parallel for
  for(unsigned int i=0;i<srng.size();i++){
    timeable::results_t results;
    double sum;
    double cur=srng[i].begin();
    dbl_param_t cur_param(cur);
    do{
      uint64_t t=function.time_func(params.iterations,cur_param, sum);
      results.push_back(new dbl_param_t(cur), t);
      cur=std::nexttoward(cur, std::numeric_limits<double>::max());
    } while(cur<srng[i].end());

    parameters_t dumpees;
    bool flag;
    do{
      flag=true;
      pthread_mutex_lock(&mutex);
      unsigned dumped=rng.range_sort(results, dumpees);
      pthread_mutex_unlock(&mutex);
      
      if( dumped*100/results.size()>20){ //20% is an arbitrary
	flag=false;
	std::cout << "Dumped: " << dumped << '/' << results.size()
		  << " Resampling ranges" << std::endl;
	pthread_mutex_lock(&mutex);
	numbers.push_back(dumpees,params.samplerate);
	rng.setup_ranges(function, numbers, params.iterations);
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
    results.free();
  }
}
