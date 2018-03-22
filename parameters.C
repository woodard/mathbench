#include <string>
#include <iostream>

#include "parameters.h"

parameters::parameters(unsigned params, const char *filename, bool nonnormals){
  std::ifstream infile(filename);
  // TODO: some error handling here
  std::string line;

  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    param_type *newone=(params==1)?
      dynamic_cast< param_type *>(new dbl_param(line)):
      dynamic_cast< param_type *>(new twodbl_param(line));
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

bool parameters::push_back(param_type *newone){
  /* don't insert duplicates so not finding a match i.e. find_if(...)==end()
     is the only time we should insert into the list */
  bool retval=std::find_if(begin(), end(),[newone](const param_type *s){
      return *s==*newone;})==end();
  if(retval)
    std::vector<param_type *>::push_back(newone);
  return retval;
}

bool parameters::push_back(double x){
  param_type *newone=new dbl_param(x);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

bool parameters::push_back(double x,double y){
  param_type *newone=new twodbl_param(x,y);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

std::ostream &operator<<(std::ostream &os, const parameters &w){
  std::cout << "---------" << std::endl << std::hexfloat;
  std::for_each(w.begin(),w.end(),
		[](auto &num){std::cout << num << std::endl;});

}

std::ostream &operator<<(std::ostream &os, const parameters::param_type &w){
     return w.write(os);
}
