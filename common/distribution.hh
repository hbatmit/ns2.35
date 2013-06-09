#ifndef DISTRIBUTION_HH
#define DISTRIBUTION_HH

#include<vector>
#include<tuple>
#include<stdint.h>

class Distribution
{
public :
	std::vector<std::tuple<double,double>> _pmf;
	
	Distribution( std::vector<double> value_vector );
	
	Distribution( std::vector<std::tuple<double,double>> pmf);
	
	double quantile( double location );
	
	Distribution compose( Distribution & other );
	
	uint32_t get_size() { return _pmf.size(); }
	
	void print( void ) ;
};

#endif
