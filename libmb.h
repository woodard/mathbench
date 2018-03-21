#ifndef __LIBMB_H__
#define __LIBMB_H_

#include <vector>
#include <cmath>
#include <string>
#include <random>

class timeable{
public:
  class param_type{
  public:
    class BAD_CONSTRUCT {};
    virtual bool operator==( const param_type &o) const =0;
    virtual bool isnormal() const =0;
    virtual std::ostream &write(std::ostream &dest) const =0;
    virtual double xval() const =0;
    virtual param_type *clone() const =0;
  };
  // ---
  class dbl_param:public param_type{
    double x;
  public:
    dbl_param(double nx):x(nx){}
    dbl_param(const dbl_param &o):x(o.x){}
    dbl_param(std::string &line){
      if( sscanf(line.c_str(),"%lf",&x) !=1 ) throw BAD_CONSTRUCT();}
    dbl_param(const char *str){
      if( sscanf(str,"%lf",&x) !=1 ) throw BAD_CONSTRUCT();}
    dbl_param(){}
    bool operator>(const dbl_param &o) const {return x>o.x;}
    bool operator>=(const dbl_param &o) const {return x>=o.x;}
    bool operator<(const dbl_param &o) const {return x<o.x;}
    bool operator<=(const dbl_param &o) const {return x<=o.x;}
    dbl_param &operator=(const dbl_param &o) {
      if(&o != this) x=o.x;
      return *this; }
    virtual bool operator==( const param_type &o)const{
      return x==dynamic_cast<const dbl_param&>(o).x;}
    virtual bool isnormal()const{ return std::isnormal(x);}
    virtual std::ostream &write(std::ostream &dest) const {return (dest << x);}
    virtual double xval() const { return x;}
    virtual param_type *clone() const { return new dbl_param(*this);}
  };
  // ---
  class twodbl_param: public param_type{
    double x,y;
  public:
    twodbl_param(double nx, double ny):x(nx),y(ny){}
    twodbl_param(const twodbl_param &o):x(o.x),y(o.y){}
    twodbl_param(std::string &line){
      if( sscanf(line.c_str(),"%lf %lf",&x,&y) !=2 )
	throw BAD_CONSTRUCT();}
    twodbl_param(const char *str){if( sscanf(str,"%lf:%lf",&x,&y) !=2 )
	throw BAD_CONSTRUCT();} // Notice: a colon rather than a space
    virtual bool operator==( const param_type &o)const{
      const twodbl_param &other=dynamic_cast<const twodbl_param &>(o);
      return x==other.x && y==other.y;}    
    virtual bool isnormal()const{ return std::isnormal(x) && std::isnormal(y);}
    virtual std::ostream &write(std::ostream &dest) const {
      return (dest << x << ',' << y);}  
    virtual double xval() const { return x;}
        virtual param_type *clone() const { return new twodbl_param(*this);}
    double yval() const { return y;}
  };
private:
  class func_type{
  public:
    virtual double call( const param_type &x)=0;
  };
  class single_func: public func_type{
    double (*func)(double);
  public:
    single_func(void *raw_func){
      func=reinterpret_cast<double (*)(double)>(raw_func); }
    virtual double call( const param_type &x){
      return func(dynamic_cast<const dbl_param&>(x).xval()); }
  };
  class twoparam_func: public func_type{
    double (*func)(double,double);
  public:
    twoparam_func(void *raw_func){
      func=reinterpret_cast<double (*)(double,double)>(raw_func);
    }
    virtual double call( const param_type &x){
      const twodbl_param &tx=dynamic_cast<const twodbl_param&>(x);
      return func(tx.xval(),tx.yval());
    }
  };
  func_type *func;
  unsigned params;
public:
  class BAD_LIBM {};
  class BAD_FNAME{};
  class BAD_FUNC{};

  timeable(const char *libmname, const char *funcname, const char *altname);
  unsigned num_params(){return params;}
  uint64_t time_func(unsigned count, const param_type &val, double &sum);
  uint64_t time_func(unsigned count, bool nonnormals, std::mt19937 &gen,
		     const param_type &min, const param_type &max, double &sum,
		     param_type *ret);
  double call(const param_type &val){ return func->call(val);}
};
std::ostream &operator<<(std::ostream &os, const timeable::param_type &w);

/* ---------------- */
class parameters:public std::vector<timeable::param_type *>{
public:
  parameters(){}
  parameters(unsigned params, const char *filename, bool nonnormals);
  bool push_back(timeable::param_type *newone);
  bool push_back(double x);
  bool push_back(double x, double y);
};
std::ostream &operator<<(std::ostream &os, const parameters &w);

/* This really can only apply to single dimension functions. The
   operations for things like areas are not quite as simple as they
   are for line segments and the concepts don't really make sense. */
class srch_rng{
  timeable::dbl_param b;
  timeable::dbl_param e;
  parameters nums;
 public:
  srch_rng(timeable::dbl_param &num, unsigned cases);
  srch_rng(){}
  srch_rng( const srch_rng &other):b(other.b),e(other.e),
			     nums(other.nums){}

  double begin() const { return b.xval();}
  double end() const { return e.xval();}
  timeable::param_type *num() const { return nums[0];}

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

void make_srngs(const parameters &nums, std::vector<srch_rng> &srng,
		unsigned cases,	bool verbose);
#endif
