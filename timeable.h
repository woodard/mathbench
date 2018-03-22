#ifndef __TIMEABLE_H__
#define __TIMEABLE_H__

#include <vector>
#include <cmath>
#include <string>
#include <random>

#include "parameters.h"

class timeable{
private:
  class func_type{
  public:
    virtual double call( const param_t &x)=0;
  };
  class single_func: public func_type{
    double (*func)(double);
  public:
    single_func(void *raw_func){
      func=reinterpret_cast<double (*)(double)>(raw_func); }
    virtual double call( const param_t &x){
      return func(dynamic_cast<const dbl_param_t&>(x).xval()); }
  };
  class twoparam_func: public func_type{
    double (*func)(double,double);
  public:
    twoparam_func(void *raw_func){
      func=reinterpret_cast<double (*)(double,double)>(raw_func);
    }
    virtual double call( const param_t &x){
      const twodbl_param_t &tx=dynamic_cast<const twodbl_param_t&>(x);
      return func(tx.xval(),tx.yval());
    }
  };
  func_type *func;
  unsigned params;
public:
  class BAD_LIBM {};
  class BAD_FNAME{};
  class BAD_FUNC{};

  typedef std::pair<param_t *, uint64_t> aresult;
  class results_t:public std::vector<aresult>{
  public:
    void free(){
      /* This class doesn't actually own the objects that are stored in its
	 vector they are just pointers back to the numbers vectors and so we
	 don't need to free the objects pointed to in its vector. */
      std::vector<aresult>::clear(); }
    void push_back(param_t *p, uint64_t time){
      aresult a=std::make_pair(p,time);
      std::vector<aresult>::push_back(a);}
  };
  
  timeable(const char *libmname, const char *funcname, const char *altname);
  unsigned num_params(){return params;}
  uint64_t time_func(unsigned count, const param_t &val, double &sum);
  uint64_t time_func(unsigned count, bool nonnormals, std::mt19937 &gen,
		     const param_t &min, const param_t &max, double &sum,
		     param_t *ret);
  double call(const param_t &val){ return func->call(val);}
};
std::ostream &operator<<(std::ostream &os, const param_t &w);

/* ---------------- */


/* This really can only apply to single dimension functions. The
   operations for things like areas are not quite as simple as they
   are for line segments and the concepts don't really make sense. */
class srch_rng{
  dbl_param_t b;
  dbl_param_t e;
  parameters_t nums;
 public:
  srch_rng(dbl_param_t &num, unsigned cases);
  srch_rng(){}
  srch_rng( const srch_rng &other):b(other.b),e(other.e),
			     nums(other.nums){}

  double begin() const { return b.xval();}
  double end() const { return e.xval();}
  param_t *num() const { return nums[0];}

  bool overlap( const srch_rng &other) const {
    if(nums.empty())
      return false;
    return (b>=other.b && b<other.e) ||
      (e>other.b && e<=other.e);
  }

  void merge( const srch_rng &other){
    if(nums.empty()){
      b=other.b;
      e=other.e;
      nums=other.nums;
    }else{
      b=std::min(b,other.b);
      e=std::max(e,other.e);
      for(auto i=other.nums.begin();i!=other.nums.end();i++)
	nums.push_back(*i);
    }
  }
};

void make_srngs(const parameters_t &nums, std::vector<srch_rng> &srng,
		unsigned cases,	bool verbose);
#endif
