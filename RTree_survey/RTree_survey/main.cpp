#include <iostream>

#include <boost/geometry.hpp>


extern int benchmark_boost_geometry(); 
extern int benchmark_boost_rtree(); 



int main(int argc, char *argv[])
{
	//benchmark_boost_geometry(); 
	benchmark_boost_rtree(); 

	system("PAUSE"); 
	return 0; 
}



