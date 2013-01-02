#ifndef DISTRIBUTION_HH
#define DISTRIBUTION_HH

#include<vector>
#include<tuple>
#include<stdint.h>

class Distribution
{
public :
	std::vector<std::tuple<uint64_t,double>> _pmf;
	
	Distribution( std::vector<uint64_t> value_vector );
	
	Distribution( std::vector<std::tuple<uint64_t,double>> pmf);
	
	uint64_t quantile( double location );
	
	Distribution compose( Distribution & other );
	
	uint32_t get_size() { return _pmf.size(); }
};

#endif
