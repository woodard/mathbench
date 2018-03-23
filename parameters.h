#ifndef __PARAMETERS_H
#define __PARAMETERS_H

#include <vector>
#include <iostream>
#include <string>
#include <cstdio>
#include <cmath>

class param_t{
 public:
  class BAD_CONSTRUCT {};
  virtual bool operator==( const param_t &o) const =0;
  virtual bool isnormal() const =0;
  virtual std::ostream &write(std::ostream &dest) const =0;
  virtual double xval() const =0;
  virtual param_t *clone() const =0;
};

class dbl_param_t:public param_t{
  double x;
 public:
 dbl_param_t(double nx):x(nx){}
 dbl_param_t(const dbl_param_t &o):x(o.x){}
  dbl_param_t(std::string &line){
    if( sscanf(line.c_str(),"%lf",&x) !=1 ) throw BAD_CONSTRUCT();}
  dbl_param_t(const char *str){
    if( sscanf(str,"%lf",&x) !=1 ) throw BAD_CONSTRUCT();}
  dbl_param_t(){}
  bool operator>(const dbl_param_t &o) const {return x>o.x;}
  bool operator>=(const dbl_param_t &o) const {return x>=o.x;}
  bool operator<(const dbl_param_t &o) const {return x<o.x;}
  bool operator<=(const dbl_param_t &o) const {return x<=o.x;}
  dbl_param_t &operator=(const dbl_param_t &o) {
    if(&o != this) x=o.x;
    return *this; }
  virtual bool operator==( const param_t &o)const{
    return x==dynamic_cast<const dbl_param_t&>(o).x;}
  virtual bool isnormal()const{ return std::isnormal(x);}
  virtual std::ostream &write(std::ostream &dest) const {return (dest << x);}
  virtual double xval() const { return x;}
  virtual param_t *clone() const { return new dbl_param_t(*this);}
};

class twodbl_param_t: public param_t{
  double x,y;
 public:
 twodbl_param_t(double nx, double ny):x(nx),y(ny){}
 twodbl_param_t(const twodbl_param_t &o):x(o.x),y(o.y){}
  twodbl_param_t(std::string &line){
    if( sscanf(line.c_str(),"%lf, %lf",&x,&y) !=2 )
      throw BAD_CONSTRUCT();}
  twodbl_param_t(const char *str){if( sscanf(str,"%lf:%lf",&x,&y) !=2 )
      throw BAD_CONSTRUCT();} // Notice: a colon rather than a space
  virtual bool operator==( const param_t &o)const{
    const twodbl_param_t &other=dynamic_cast<const twodbl_param_t &>(o);
    return x==other.x && y==other.y;}    
  virtual bool isnormal()const{ return std::isnormal(x) && std::isnormal(y);}
  virtual std::ostream &write(std::ostream &dest) const {
    return (dest << x << ',' << y);}  
  virtual double xval() const { return x;}
  virtual param_t *clone() const { return new twodbl_param_t(*this);}
  double yval() const { return y;}
};
  
class parameters_t:public std::vector<param_t *>{
public:
  class BAD_NUMFILE{};
  parameters_t(){}
  parameters_t(unsigned params, const char *filename, bool nonnormals);
  bool push_back(param_t *newone);
  bool push_back(double x);
  bool push_back(double x, double y);
  /* This assumes that the parameters which are coming in in the dumpee's vector 
     are owned by someone else. 
     A samplerate of 1 assumes that every value is copied into the list of 
     parameters while a sample rate of 100 means that 1/100 values are added to 
     the list */
  void push_back(const parameters_t &dumpees, unsigned samplerate=1);
};
std::ostream &operator<<(std::ostream &os, const parameters_t &w);

#endif
