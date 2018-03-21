#include <stdio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/time.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <random>

#include "libmb.h"

parameters::parameters(unsigned params, const char *filename, bool nonnormals){
  std::ifstream infile(filename);
  // TODO: some error handling here
  std::string line;

  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    timeable::param_type *newone=(params==1)?
      dynamic_cast< timeable::param_type *>(new timeable::dbl_param(line)):
      dynamic_cast< timeable::param_type *>(new timeable::twodbl_param(line));
    if(nonnormals==false && !newone->isnormal()){
      std::cout << "Discarding nonnormal: " << line << std::endl;
      delete newone;
      continue;
    }
    // add to the list if not a duplicate.
    if(!push_back(newone)){ 
      std::cout << "Discarding duplicate: " << line << std::endl;
      delete newone;
    }
  }
}

bool parameters::push_back(timeable::param_type *newone){
  /* don't insert duplicates so not finding a match i.e. find_if(...)==end()
     is the only time we should insert into the list */
  bool retval=std::find_if(begin(), end(),
			   [newone](const timeable::param_type *s){
			     return *s==*newone;}
			   )==end();
  if(retval)
    std::vector<timeable::param_type *>::push_back(newone);
  return retval;
}

bool parameters::push_back(double x){
  timeable::param_type *newone=new timeable::dbl_param(x);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

bool parameters::push_back(double x,double y){
  timeable::param_type *newone=new timeable::twodbl_param(x,y);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

void make_srngs(const parameters &nums, std::vector<srch_rng> &srng,
		 unsigned cases, bool verbose){
  for( auto num=nums.begin();num!=nums.end();num++){
    srch_rng newone(*dynamic_cast<timeable::dbl_param*>(*num),cases);
    bool flag=false;
    for( auto sr=srng.begin();sr!=srng.end();sr++){
      // if the ranges overlap
      if( sr->overlap(newone)){
	sr->merge(newone);
	flag=true;
	if(verbose)
	  std::cout << "Combining " << std::hexfloat << *num
		    << " into a test range with " << std::hexfloat
		    << sr->num() << " since they overlap" << std::endl;
	break;
      }
    }
    if(!flag){ // create a new search range
      srng.push_back(newone);
      if(verbose)
	std::cout << "New search range for " << std::hexfloat << *num << " ("
		  << newone.begin() << '-' << newone.end() << ')' << std::endl;
    }
  }
}

srch_rng::srch_rng(timeable::dbl_param &num, unsigned cases):b(num),e(num){
  nums.push_back(new timeable::dbl_param(num));
  for(unsigned i=0;i<cases;i++){
    b=std::nexttoward(b.xval(),-std::numeric_limits<double>::max());
    e=std::nexttoward(e.xval(), std::numeric_limits<double>::max());
  }
}

/* make this go away and use PAPI or Caliper or something */
static inline void Gettimeofday(struct timeval &ts){
  if (gettimeofday (&ts, NULL) != 0) {
    fprintf (stderr, "Unable to get time of day, exiting\n");
    exit (1);
  }
}

timeable::timeable(const char *libmname, const char *funcname,
			       const char *altname){
  try{
    void *libm=dlmopen(LM_ID_NEWLM,libmname , RTLD_LAZY);
    if(libm==NULL)
      if(std::string(libmname)=="libm.so"){
	libmname="libm.so.6";
	libm=dlmopen(LM_ID_NEWLM,libmname , RTLD_LAZY);
	if(libm==NULL)
	  throw BAD_LIBM();
      }
    void *raw_func;
    raw_func=dlsym(libm, altname!=NULL?altname:funcname);
    if(raw_func==NULL)
      throw BAD_FNAME();

    std::string fn(funcname);
    if( fn=="tan"   || fn=="cos"  || fn=="sin"   || fn=="acosh" || fn=="acos" ||
	fn=="asinh" || fn=="asin" || fn=="atanh" || fn=="cosh"  || fn=="exp2" ||
	fn=="log2"  || fn=="log"  || fn=="rint"  || fn=="sinh"  || fn=="sqrt" ||
	fn=="tanh"  || fn=="trunc"|| fn=="exp" ){
      params=1;
      func=new single_func(raw_func);
    } else if( fn=="pow"){
      params=2;
      func=new twoparam_func(raw_func);
    } else
      throw BAD_FUNC();
  }
  catch(BAD_LIBM &){
    std::cerr << "Loading of libm implementation failed: " << dlerror()
	      << std::endl;
    exit(1);
  }
  catch(BAD_FNAME &){
    std::cerr << "Could not locate the function "
	      << (altname!=NULL?altname:funcname) << ' '
	      << dlerror() << std::endl;
    exit(1);
  }
  catch(BAD_FUNC &){
    std::cerr << "Unknown function " << (altname!=NULL?altname:funcname)
	      << std::endl;
    exit(1);
  }
}




uint64_t timeable::time_func(unsigned count, bool nonnormals, std::mt19937 &gen,
			    const param_type &min, const param_type &max,
			    double &sum, param_type *ret){
  double dmin=min.xval();
  double dmax=max.xval();
  std::uniform_real_distribution<double> dis(dmin,dmax);
  double x;
  do{
    x=dis(gen);
  }while(nonnormals==true || !std::isnormal(x));

  if(num_params()==1)
    ret=new dbl_param(x);
  else{
    dmin=dynamic_cast<const twodbl_param&>(min).yval();    
    dmax=dynamic_cast<const twodbl_param&>(max).yval();
    double y;
    std::uniform_real_distribution<double> dis2(dmin,dmax);
    do{
      y=dis2(gen);
    }while(nonnormals==true || !std::isnormal(y));
    ret=new twodbl_param(x,y);
  }
  return time_func(count,*ret,sum);
}

uint64_t timeable::time_func(unsigned count, const param_type &val, double &sum){
  sum=0.0;
  struct timeval start_ts, end_ts;

  Gettimeofday(start_ts);
  for (int i=1; i <= count; i++)
      sum += func->call(val);
  Gettimeofday(end_ts);

  /* Calculate run time */
  uint64_t useconds;
  suseconds_t usecs = end_ts.tv_usec-start_ts.tv_usec;
  time_t secs = end_ts.tv_sec-start_ts.tv_sec;
  useconds = secs*1000000+usecs;
  return useconds;
}

std::ostream &operator<<(std::ostream &os, const timeable::param_type &w){
     return w.write(os);
}

std::ostream &operator<<(std::ostream &os, const parameters &w){
  std::cout << "---------" << std::endl << std::hexfloat;
  std::for_each(w.begin(),w.end(),
		[](auto &num){std::cout << num << std::endl;});

}
