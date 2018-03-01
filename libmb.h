#ifndef __LIBMB_H__
#define __LIBMB_H_

double (*getFunc(char *optarg))(double);
void read_numbers(char *filename, bool nonnormals,
		  std::vector< double> &numbers);

class srch_rng{
  double b;
  double e;
  std::vector<double> nums;
 public:
  srch_rng(double num, unsigned cases);
  srch_rng(){}
  srch_rng( const srch_rng &other):b(other.b),e(other.e),
			     nums(other.nums){}

  double begin() const { return b;}
  double end() const { return e;}
  double num() const { return nums[0];}

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

void make_srngs( const std::vector<double> &numbers,
		 std::vector<srch_rng> &srng, unsigned cases, bool verbose);
#endif
