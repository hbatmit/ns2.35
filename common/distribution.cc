#include "distribution.hh"
#include<algorithm>

Distribution::Distribution( std::vector<uint64_t> value_vector ) :
	_pmf( std::vector<std::tuple<uint64_t,double>>( value_vector.size() ) )
{
	/* Append probability masses and write into new vector */
	std::transform( value_vector.begin(), value_vector.end(), _pmf.begin(), [&] (uint64_t x) { return std::tuple<uint64_t,double>( x, 1.0/value_vector.size() ); } );

	/*Sort for finding quantiles later */
	std::sort( _pmf.begin(), _pmf.end(),
	           [&] (const std::tuple<uint64_t,double> & x, const std::tuple<uint64_t,double> & y)
	           { return std::get<0>(x) < std::get<0>(y); });
}

Distribution::Distribution( std::vector<std::tuple<uint64_t,double>> pmf ) :
	_pmf( pmf )
{}

uint64_t Distribution::quantile( double location )
{
	if ( _pmf.empty() ) return 0;
	double cumulative = 0;
	for (uint32_t i=0; i < _pmf.size(); i++ ) {
		/* location crosses cumulative for first time */
		if ( location < cumulative ) {
			return std::get<0>( _pmf.at( i ) );
		} else {
			 cumulative += std::get<1>( _pmf.at(i) );
		}
	}
	return std::get<0>( _pmf.at( _pmf.size() - 1 ) );
}

Distribution Distribution::compose( Distribution & other )
{
	/*Compute normalizing factors */
	double self_prob = ( (double) this->get_size() ) / (this->get_size() + other.get_size());
	double other_prob= 1 - self_prob;

	/* Rewrite pmf for both distributions */
	std::vector<std::tuple<uint64_t,double>> self_new_pmf(  this->get_size() );
	std::vector<std::tuple<uint64_t,double>> other_new_pmf( other.get_size() );
	std::transform( _pmf.begin(), _pmf.end(), self_new_pmf.begin(),
	                [&] ( const std::tuple<uint64_t,double> & x)
	                { return std::tuple<uint64_t,double> (std::get<0>(x), std::get<1>(x)*self_prob); } );
	std::transform( other._pmf.begin(), other._pmf.end(), other_new_pmf.begin(),
	                [&] ( const std::tuple<uint64_t,double> & x)
	                { return std::tuple<uint64_t,double> (std::get<0>(x), std::get<1>(x)*other_prob); } );

	/*Merge sort the two lists */
	std::vector<std::tuple<uint64_t,double>> merged_pmf( this->get_size() + other.get_size() );
	std::merge( self_new_pmf.begin(), self_new_pmf.end(), other_new_pmf.begin(), other_new_pmf.end(),
	            merged_pmf.begin(),
	            [&] ( const std::tuple<uint64_t,double> & x, const std::tuple<uint64_t,double> & y)
	            { return std::get<0>(x) < std::get<0>(y); } );

	return Distribution( merged_pmf );
}

void Distribution::print( void )
{
	fprintf( stderr, "Begin dist :\n");
	std::for_each( _pmf.begin(), _pmf.end(),
	               [&] ( const std::tuple<uint64_t,double> x )
	               { fprintf( stderr, "(%lu,%f) ", std::get<0>(x), std::get<1>(x) );});
	fprintf( stderr, "\n End dist\n");
}
