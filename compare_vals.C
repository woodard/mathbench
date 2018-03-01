#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <dlfcn.h>

#include <vector>
#include <utility>
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>

#include "libmb.h"

bool verbose=false;
bool nonnormals=false;
double (*func)(double)=&exp;

int main(int argc, char **argv){
  int c=0;
  static struct option long_options[] = {
    {"altfuncname",    required_argument, 0, 'a'},
    {"cases",          required_argument, 0, 'c'},
    {"dumpnumbers",    no_argument,       0, 'd'},
    {"function",       required_argument, 0, 'f'},
    {"nonnormals",     no_argument,       0, 'n'},
    {"targetd",        no_argument,       0, 't'},
    {"verbose",        no_argument,       0, 'v'},
    {0,                0,                 0,  0 }
  };
  const char *funcname="exp";
  const char *altfuncname=NULL;
  unsigned cases=20000;
  bool targeted=false;
  bool dumpnums=false;

  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    c = getopt_long(argc, argv, "a:c:df:ntv",
		    long_options, &option_index);
    switch (c) {
    case 'a':
      altfuncname=optarg;
      break;
    case 'c':
      sscanf(optarg,"%ld",&cases);
      break;
    case 'd':
      dumpnums=true;
      break;
    case 'f':
      func=getFunc(optarg);
      funcname=optarg;
      break;
    case 'n':
      nonnormals=true;
      break;
    case 't':
      targeted=true;
      break;
    case 'v':
      verbose=true;
      break;
    case -1:
      break; // also terminates the while loop
    }
  }

  std::vector< double> numbers;
  read_numbers(argv[optind],nonnormals,numbers);

  void *altlibm=dlmopen(LM_ID_NEWLM, argv[optind+1], RTLD_LAZY);
  if( altlibm==NULL){
    std::cerr << "Loading of alternative libm implementation failed: "
	      << dlerror() << std::endl;
    exit(3);
  }

  if(altfuncname==NULL)
    altfuncname=funcname;
  double (*altfunc)(double)=
    reinterpret_cast<double (*)(double)>(dlsym(altlibm,altfuncname));
  
  if( altfunc==NULL)
    exit(4);

  if(targeted){
    std::vector<srch_rng> srng;
    make_srngs( numbers, srng, cases, verbose);

    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    class res{
    public:
      srch_rng &rng;
      unsigned bad;
      unsigned tested;
      double worst;
      res( srch_rng &r, unsigned b, unsigned t, double w):
	rng(r),bad(b),tested(t),worst(w){}
      res( const res &o):
	rng(o.rng),bad(o.bad),tested(o.tested),worst(o.worst){}
    };
    std::vector< res > results;
 #pragma omp parallel for
    for(unsigned int i=0;i<srng.size();i++){
      double cur=srng[i].begin();
      unsigned int bad_ones=0;
      unsigned int tested=0;
      double worst=0.0;
      do{
	tested++;
	double r1=func(cur);
	double r2=altfunc(cur);
	if(r1!=r2){
	   bad_ones++;
	   worst=std::max(worst, abs(r1-r2));
	   if(dumpnums){
	     pthread_mutex_lock(&m);
	     if(std::find(numbers.begin(),numbers.end(),cur)
		==numbers.end())
	       numbers.push_back(cur);
	     pthread_mutex_unlock(&m);
	   }
	}
	cur=std::nexttoward(cur, std::numeric_limits<double>::max());
      } while(cur<srng[i].end());
      if( bad_ones){
	pthread_mutex_lock(&m);
	results.push_back( res(srng[i], bad_ones, tested, worst));
	pthread_mutex_unlock(&m);
      }
    }
    for( auto cur=results.begin();cur!=results.end();cur++){
      std::cout << "Between: " << std::hexfloat
		<< cur->rng.begin() << '-' << cur->rng.end() << '\t'
		<< cur->bad << " bad values\tmax error: "
		<< cur->worst << std::endl;
    }
    std::cout << "-------" << std::endl;
  }

  unsigned int bad_ones=0;
  std:: cout << funcname << "(x) = glibc, " << argv[optind+1] << std::endl;
  for( unsigned int i=0;i<numbers.size();i++){
    double r1=func(numbers[i]);
    double r2=altfunc(numbers[i]);
    if(r1!=r2){
      bad_ones++;
      std::cout << funcname << '(' << std::hexfloat << numbers[i]
		<< ") = " << r1 << ", "	<< r2 << " difference: "
		<< abs(r1-r2) << std::endl;
    }
  }
  std::cout << bad_ones << '/' << numbers.size() << " failed ["
	    << std::setprecision(4) << std::defaultfloat
	    << bad_ones*100.0/numbers.size() << "%]" << std::endl;
  exit(0);
}
