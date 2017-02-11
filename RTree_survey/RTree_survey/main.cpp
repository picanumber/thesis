#include <iostream>
#include "optimization.h"

extern int benchmark_boost_rtree(); 
extern int run_rtree_operations(int dataset, int numElems, int numQs, int qTypes, int minNodes, int maxNodes, int splitType);
extern int benchmark_boost_rtree_real(); 
extern int benchmark_boost_rtree_synth(); 

int main(int argc, char *argv[])
{
	int k;
	std::cout << "waiting for you to set the affinity\n";
	std::cin >> k;

	//benchmark_boost_rtree(); 
	//std::cout << run_rtree_operations(DATASET_SYTH2D, 50'000, 10'000, Q_OVERLAPS, 4, 8, LOAD_LIN) << std::endl;

	benchmark_boost_rtree_real();
	benchmark_boost_rtree_synth();
	
	system("PAUSE"); 
	return 0; 
}
