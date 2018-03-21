#include <stdlib.h>
#include <getopt.h>

#include <vector>
#include <utility>
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>

#include "libmb.h"

bool verbose=false;
bool nonnormals=false;

bool close_enough(bool oneulp, double r1, double r2){
  if(r1==r2) return true;
  if(!oneulp) return false;
  double r1_plus=std::nexttoward(r1, std::numeric_limits<double>::max());
  double r1_minus=std::nexttoward(r1, -std::numeric_limits<double>::max());
  if(r1_plus==r2 || r1_minus==r2) return true;
  return false;
}

int main(int argc, char **argv){
  int c=0;
  static struct option long_options[] = {
    /* While the standard has been 1/2 ULP for x86_64 for some time it
       has been argued that it is acceptable to only maintain 1 ULP of
       accuracy in the tracendental functions being explored. Therefore, 
       this filters out the cases where the result is within 1ULP as opposed 
       to 1/2 ULP by allowing values which are adacent to the axxurate one. */
    {"oneulp",         required_argument, 0, '1'},
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
  const char *altfname=NULL;
  const char *libmname="libm.so";
  const char *altlibmname=NULL;

  unsigned cases=20000;
  bool targeted=false;
  bool dumpnums=false;
  bool oneulp=false;

  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    c = getopt_long(argc, argv, "1a:c:df:ntv",
		    long_options, &option_index);
    switch (c) {
    case '1':
      oneulp=true;
      break;
    case 'a':
      altfname=optarg;
      break;
    case 'c':
      sscanf(optarg,"%ld",&cases);
      break;
    case 'd':
      dumpnums=true;
      break;
    case 'f':
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

  timeable func1(libmname, funcname, NULL);
  timeable func2(altlibmname, funcname, altfname);
  parameters numbers(func1.num_params(), argv[optind], nonnormals);

  if(targeted){
    if( func1.num_params()==2)
      exit(2);
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
      unsigned int bad_ones=0;
      unsigned int tested=0;
      double worst=0.0;
      double cur=srng[i].begin();
      do{
 	tested++;
 	timeable::dbl_param cur_val(cur);
 	double r1=func1.call(cur_val);
 	double r2=func2.call(cur_val);
 	bool close=close_enough(oneulp,r1,r2);
 	if(r1!=r2 && close && verbose)
 	  std::cout << funcname << '(' << std::hexfloat << cur << ") = "
 		    << r1 << ", "	<< r2 << " difference: "
 		    << abs(r1-r2) << " is " << (r1>r2?'+':'-')
 		    << "1 ULP" << std::endl;
 	if(!close){
 	  bad_ones++;
 	  worst=std::max(worst, abs(r1-r2));
 	  if(dumpnums){
 	    pthread_mutex_lock(&m);
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
      std::cout << "Between: " << std::hexfloat	<< cur->rng.begin()
    		<< " - " << cur->rng.end() << '\t' << cur->bad
    		<< " bad values\tmax error: "
    		<< cur->worst << std::endl;
    }
    std::cout << "-------" << std::endl;
  } // end of targeted mode

  unsigned int bad_ones=0;
  std::cout << funcname << "(x) = glibc, " << argv[optind+1] << std::endl;
  for( unsigned int i=0;i<numbers.size();i++){
    double r1=func1.call(*numbers[i]);
    double r2=func2.call(*numbers[i]);
    if(!close_enough(oneulp, r1, r2)){
      bad_ones++;
      std::cout << funcname << '(' << std::hexfloat
		<< *numbers[i] << ") = " << r1 << ", "	<< r2 << " difference: "
		<< abs(r1-r2) << std::endl;
    }
  }
  std::cout << bad_ones << '/' << numbers.size() << " failed" << std::endl;
  if(dumpnums)
    std::cout << "---------" << std::endl << numbers << std::endl;

  exit(0);
}
