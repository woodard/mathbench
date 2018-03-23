#include <string>
#include <fstream>
#include <algorithm>

#include "parameters.h"

parameters_t::parameters_t(unsigned params, const char *filename, bool nonnormals){
  std::ifstream infile(filename);
  if(!infile)
    throw BAD_NUMFILE();
  std::string line;

  while (std::getline(infile, line)){
    if(line[0]=='#')
      continue;
    param_t *newone=(params==1)?
      dynamic_cast< param_t *>(new dbl_param_t(line)):
      dynamic_cast< param_t *>(new twodbl_param_t(line));
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

bool parameters_t::push_back(param_t *newone){
  /* don't insert duplicates so not finding a match i.e. find_if(...)==end()
     is the only time we should insert into the list */
  bool retval=std::find_if(begin(), end(),[newone](const param_t *s){
      return *s==*newone;})==end();
  if(retval)
    std::vector<param_t *>::push_back(newone);
  return retval;
}

bool parameters_t::push_back(double x){
  param_t *newone=new dbl_param_t(x);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

bool parameters_t::push_back(double x,double y){
  param_t *newone=new twodbl_param_t(x,y);
  bool retval=push_back(newone);
  if(!retval)
    delete newone;
  return retval;
}

void parameters_t::push_back(const parameters_t &dumpees,
			     unsigned samplerate){
  for(int i=0;i<dumpees.size();i+=samplerate)
    /* since dumpees doesn't own its parameters they must be cloned before 
       being inserted into the array of numbers */
    push_back(dumpees[i]->clone());
}


std::ostream &operator<<(std::ostream &os, const parameters_t &w){
  std::cout << "---------" << std::endl << std::hexfloat;
  std::for_each(w.begin(),w.end(),
		[](auto &num){std::cout << *num << std::endl;});

}

std::ostream &operator<<(std::ostream &os, const param_t &w){
     return w.write(os);
}
