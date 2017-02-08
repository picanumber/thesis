# =============================================================================
# Standard Python modules
# =============================================================================
import os, sys, time
import pdb
import time
import multiprocessing

# =============================================================================
# Extension modules
# =============================================================================
import optimize_rtree
from pyswarm import pso

# =============================================================================
# 
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
        x1 = int(x[1])
        x0 = int(0.1 * x[0] * x1)
        if x0 < 1: x0 = 1

        a = '{}:{}'.format(x0, x1)
        if a in used_solutions:
            print 'reusing solution ({}, {}) for ({}, {})'.format(x0, x1, x[0], x[1])
            return used_solutions[a]

        f = x1**4 - 2*x0*x1**2 + x0**2 + x1**2 - 2*x1 + 5
        return f
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
# 
# =============================================================================
def solve_optimization_problem(
    dataset, rtree_size, query_size, query, split_type): 
    """
    creates the objective function and optimization problem
    """
    objfunc = get_objective_func('syth2d', 50000, 10000, 'within', split_type)
    lb = [1, 4]
    ub = [5, 128]
    xopt, fopt = pso(objfunc, lb, ub, minstep = 1)
    return (fopt, xopt, split_type)

# =============================================================================
# used by the multithreaded module
# =============================================================================
def set_up_optimization(dataset, rtree_size, query_size, query): 
    """ returns an optimization setup
    """
    def opt_fun(split_type): 
        return solve_optimization_problem(dataset, rtree_size, query_size, query, split_type)

    return opt_fun

# =============================================================================
# 
# =============================================================================
def single_threaded():
    """entry point for the application"""
    lts = [] # list of tuples
    for split in ['lin', 'qdrt', 'rstar', 'bulk']: 
        lts.append(solve_optimization_problem('real2d', 50000, 10000, 'within', split))

    with open('pyswarm_outpu.txt', 'w') as outfile:
        for rec in lts:
            outfile.write('opt={} | minNod={}, maxNod={} | split={}\n'.format(
                rec[0], rec[1][0], rec[1][1], rec[2]))

# =============================================================================
# 
# =============================================================================
def call_opt_task(tup):
    return tup[0](tup[1])

# =============================================================================
# 
# =============================================================================
def multi_threaded():
    pool = multiprocessing.Pool()
    op      = set_up_optimization('real2d', 50000, 10000, 'within')
    tasks   = [(op, 'lin'), (op, 'qdrt'), (op, 'rstar'), (op, 'bulk')]
    outputs = pool.map(call_opt_task, tasks)

    with open('pyswarm_outpu.txt', 'w') as outfile:
        for rec in outputs:
            outfile.write('opt={} | minNod={}, maxNod={} | split={}\n'.format(
                rec[0], rec[1][0], rec[1][1], rec[2]))

    return 0

# =============================================================================
# 
# =============================================================================
def main():
    multi_threaded(); 

# =============================================================================
if __name__ == '__main__':
    start = time.time()
    main()
    secs = time.time()-start
    print 'Optimization duration: ', (secs / 60.), ' minutes.'



