#include <math.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>

#include "libmb.h"

double (*getFunc(char *optarg))(double){
  double (*func)(double);
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
  return func;
}

void read_numbers(char *filename, bool nonnormals,
		  std::vector< double> &numbers){
  std::ifstream infile(filename);
  // TODO: some error handling here
  std::string line;

  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    double a;
    if( sscanf(line.c_str(),"%lf",&a) == 0)
      continue;
    if( std::find(numbers.begin(), numbers.end(), a) != numbers.end())
      std::cout << "Discarding duplicate: " << line << std::endl;
    else
      if(isnormal(a) || nonnormals==true)
	numbers.push_back(a);
      else
	std::cout << "Discarding nonnormal: " << line << std::endl;
  }
}

void make_srngs( const std::vector<double> &numbers,
		 std::vector<srch_rng> &srng, unsigned cases, bool verbose){
  for( auto num=numbers.begin();num!=numbers.end();num++){
    srch_rng newone(*num,cases);
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

srch_rng::srch_rng(double num, unsigned cases):b(num),e(num){
  nums.push_back(num);
  for(unsigned i=0;i<cases;i++){
    b=std::nexttoward(b,-std::numeric_limits<double>::max());
    e=std::nexttoward(e, std::numeric_limits<double>::max());
  }
}
