# =============================================================================
# Standard Python modules
# =============================================================================
import os, sys, time
import pdb
import time

# =============================================================================
# Extension modules
# =============================================================================
import optimize_rtree
from pyswarm import pso

# =============================================================================
def get_max_nodes(mn):
    """ 
    """
    return int(mn)

# =============================================================================
def get_min_nodes(min_node_perc, max_nodes):
    """
    """
    x0 = int(0.1 * min_node_perc * max_nodes)
    if x0 < 1: x0 = 1
    return x0

# =============================================================================
def print_header(dataset, numElems, numQs, qType, filename, minutes):
    """ prints info on the experiment in filename 
    """
    info = 'dataset={}, numElems={}, numQs={}, qType={}\n'.format(
        dataset, numElems, numQs, qType)
    subscr = '=' * (len(info) - 1)

    with open(filename, 'w') as outfile: 
        outfile.write(
            '\n\n\t*** R-tree optimization with pyswarm ({} minutes) ***\n\n'.
            format(minutes))
        outfile.write(subscr + '\n')
        outfile.write(info)
        outfile.write(subscr + '\n\n')

# =============================================================================
def get_objective_func(dataset, numElems, numQs, qType, split):
    """
    dataset  : one of ['real2d', 'syth2d', 'real3d', 'syth3d']
    numElems : the population of the rtree
    numQs    : the number of queries
    qType    : ['within', 'contains', 'covered_by', 'covers', 'disjoint', 
                'intersects', 'knn', 'overlaps']
    """
    used_solutions = {}
    
    ds_num = {
        'real2d' : 1, 'syth2d' : 2, 'real3d' : 3, 'syth3d' : 4
    }[dataset]

    query = {
        'within' : 0, 'contains' : 1, 'covered_by' : 2, 'covers' : 3,
        'disjoint' : 4, 'intersects' : 5, 'knn' : 6, 'overlaps' : 7
    }[qType]

    split_algo = { 
        'lin' : 1, 'qdrt' : 2, 'rstar' : 3, 'bulk' : 4 
    }[split]

    def rtree_load_and_query(x, *args, **kwargs): 
        """
        """
        x1 = get_max_nodes(x[1])
        x0 = get_min_nodes(x[0], x1)

        a = '{}:{}'.format(x0, x1)
        if a in used_solutions:
            print 'reusing solution ({}, {}) for ({}, {})'.format(x0, x1, x[0], x[1])
            return used_solutions[a]

        # 2 lines below only for debug purposes
        #f = x1**4 - 2*x0*x1**2 + x0**2 + x1**2 - 2*x1 + 5
        #return f
        f = optimize_rtree.objective_function(dataset=ds_num, numElems=numElems, 
                                              numQs=numQs, qType=query, minNodes=x0, 
                                              maxNodes=x1, splitType=split_algo)
        
        if f < 0: # TODO: make constraints work
            print 'somebody failed, f returned -1'
            f = 100000
        
        used_solutions[a] = f 
        return f

    return rtree_load_and_query

# =============================================================================
def solve_optimization_problem(
    dataset, rtree_size, query_size, query, split_type): 
    """
    creates the objective function and optimization problem
    """
    objfunc = get_objective_func(dataset, rtree_size, query_size, query, split_type)
    lb = [1, 4]   # lb = 1 tenth  (0.1) -   4 nodes
    ub = [5, 128] # ub = 5 tenths (0.5) - 128 nodes
    
    xopt, fopt = pso(objfunc, lb, ub, minstep = 1, debug = True)
    x1 = get_max_nodes(xopt[1])
    x0 = get_min_nodes(xopt[0], x1)
    rec = 'optimum latency = {} | nodes = ({}, {}) | split = {}'.format(fopt, x0, x1, split_type) 
    return rec

# =============================================================================
def single_threaded(dataset, numElems, numQs, qType, filename):
    """entry point for the application"""
    start = time.time()

    lts = [] # list of tuples
    for split in ['lin', 'qdrt', 'rstar', 'bulk']: 
        lts.append(solve_optimization_problem(dataset, numElems, numQs, qType, split))

    secs = time.time() - start

    print_header(dataset, numElems, numQs, qType, filename, (secs // 60))
    with open(filename, 'a') as outfile:
        for rec in lts: outfile.write(rec + '\n')

    return 0

# =============================================================================
def main():
    single_threaded('real2d', 10000, 5000, 'within', 'pyswarm_single_threaded_100000_50000.txt'); 

# =============================================================================
if __name__ == '__main__':
    raw_input("Press Enter to proceed...")
    main()



