#pragma once

int test_optimizer(int, char const* a, char const* b, char const* c); 


/* # magic numbers
----------------*/
// below arbitrary constants are assigned to operations; this should be more structured, but 
// it serves  as a simple  layer to communicate  information to python w/o exporting classes

// dataset types
#define DATASET_REAL2D 1
#define DATASET_SYTH2D 2
#define DATASET_REAL3D 3
#define DATASET_SYTH3D 4

// query types
#define Q_WITHIN      0
#define Q_CONTAINS    1
#define Q_COVERED_BY  2
#define Q_COVERS      3
#define Q_DISJOINT    4 
#define Q_INTERSECTS  5
#define Q_KNN         6
#define Q_OVERLAPS    7 

// split types
#define LOAD_LIN    1
#define LOAD_QDRT   2
#define LOAD_RSTAR  3
#define LOAD_BULK   4

/* ~ magic numbers
----------------*/

// result is return in milliseconds run time
int run_rtree_operations(
	int dataset, int numElems, int numQs, int qType,  // parameters that set the problem
	int minNodes, int maxNodes, int splitType         // parameters to optimize
); 



