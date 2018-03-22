#include <algorithm>

#include "ranges.h"

unsigned ranges::range_sort(const timeable::results_t &results,
			    parameters_t &dumpees){
  unsigned dumped=0;
  std::for_each(results.begin(),results.end(),
		[this,dumped,dumpees](auto &result){
      auto arange=std::find_if(begin(),end(),[result](auto &range){
	  return result.second>=range.min && result.second<=range.max;});
      if(arange!=end())
	arange->count++;
      else{
	// didn't fit into a range
	dumped++;
	dumpees.push_back(result.first);
      }
    });
  return dumped;
}

void ranges::setup_ranges(timeable &tm, parameters &numbers,
                         unsigned iterations){
  result_t results;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#pragma omp parallel for
  for( int i=0;i<numbers.size();i++){
    timeable::param_type *b=numbers[i];
    double sum;
    uint64_t time=tm.time_func(iterations, *b, sum);

    pthread_mutex_lock(&mutex);
    results.push_back(b,time);
    pthread_mutex_unlock(&mutex);
  }

  // end parallel region
  std::sort(results.begin(), results.end(),
            [](auto &left, auto &right) { return left.second < right.second; }
            );

  unsigned count=1;
  unsigned sum=results[0].second,begin=0;
  for( unsigned i=1;i<results.size()-1;i++){ // intentionally starts at 1
    auto lowgap=results[i].second-results[i-1].second;
    auto highgap=results[i+1].second-results[i].second;
    auto partmean=sum/count/20; // 20=5% which is arbitrary but works
    if( (lowgap<=highgap && highgap<partmean) || lowgap<=partmean ) { 
      count++;
      sum+=results[i].second;
    } else { // closer to next value
      if(count>2)
        //if it is this small it probably isn't a plateau it is a transition
        std::vector<range>::push_back(range(begin,i-1,count,sum));
      begin=i;
      count=1;
      sum=results[i].second;
    }
  }
  std::vector<range>::push_back(range(begin,results.size()-1,count,sum));
  std::for_each(std::vector<range>::begin(),std::vector<range>::end(),
               [results](auto &it){
                 it.min=results[it.begin].second;
                 it.max=results[it.end].second;});
}

std::ostream &operator<<(std::ostream &os, const ranges &r){
  std::cout << std::endl << r.size() << " Ranges: " << std::endl;
  std::for_each(r.begin(),r.end(),
               [](auto &it){
                 std::cout << it.begin << '-' << it.end << " c:"
                           << it.count << "\tm:" << it.sum/it.count
                           << " \tr:" << it.min << ':' << it.max << ' '
                           << std::endl;});
}

