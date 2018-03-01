#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <dlfcn.h>

#include <vector>
#include <iostream>
#include "libmb.h"

bool verbose=false;
bool nonnormals=false;
double (*func)(double)=&exp;

int main(int argc, char **argv){
  int c=0;
  static struct option long_options[] = {
    {"function",       required_argument, 0, 'f'},
    {"nonnormals",     no_argument,       0, 'n'},
    {"verbose",        no_argument,       0, 'v'},
    {0,                0,                 0,  0 }
  };
  const char *funcname="exp";
  while (c!=-1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    c = getopt_long(argc, argv, "f:nv",
		    long_options, &option_index);
    switch (c) {
    case 'f':
      func=getFunc(optarg);
      funcname=optarg;
      break;
    case 'v':
      verbose=true;
      break;
    case 'n':
      nonnormals=true;
      break;
    case -1:
      break; // also terminates the while loop
    }
  }

  std::vector< double> numbers;
  read_numbers(argv[optind],nonnormals,numbers);

  void *altlibm=dlmopen(LM_ID_NEWLM, argv[optind+1], RTLD_LAZY);
  if( altlibm==NULL)
    exit(3);
  double (*altfunc)(double)=
    reinterpret_cast<double (*)(double)>(dlsym(altlibm,funcname));
  if( altfunc==NULL)
    exit(4);

  std:: cout << funcname << "(x) = glibc, " << argv[optind+1] << std::endl;
  // #pragma omp parallel for
  for( int i=0;i<numbers.size();i++){
    double r1=func(numbers[i]);
    double r2=altfunc(numbers[i]);
    if(r1!=r2)
      std::cout << funcname << '(' << std::hexfloat << numbers[i]
		<< ") = " << r1 << ", "	<< r2 << " difference: " << abs(r1-r2)
		<< std::endl;
  }
  exit(0);
}
