#ifndef __RANGES_H__
#define __RANGES_H__

#include <utility>
#include <cstdint>
#include <vector>

#include "parameters.h"
#include "timeable.h"

class range_t{
public:
  unsigned begin;
  unsigned end;
  unsigned count;
  unsigned sum;
  uint64_t min,max;

  range_t(){}
  range_t(unsigned b, unsigned e, unsigned c, unsigned s):
    begin(b),end(e),count(c),sum(s){}
};

class ranges:public std::vector<range_t>{
public:  
  ranges(timeable &tm, parameters_t &numbers, unsigned iterations){
    setup_ranges(tm,numbers, iterations); }
  void setup_ranges(timeable &tm, parameters_t &numbers, unsigned iterations);
  unsigned range_sort(const timeable::results_t &results,parameters_t &dumpees);
  void clear_counts(){
    std::for_each(begin(),end(),[](range_t &e){e.count=0;});}

  friend std::ostream &operator<<(std::ostream &os, const ranges &r);
};

#endif
