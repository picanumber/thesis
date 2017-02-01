#include <iostream>

extern int benchmark_boost_rtree(); 

int main(int argc, char *argv[])
{
	int k;
	std::cout << "waiting for you to set the affinity\n";
	std::cin >> k;

	benchmark_boost_rtree(); 

	system("PAUSE"); 
	return 0; 
}



