# =============================================================================
# Standard Python modules
# =============================================================================
import os, sys, time
import pdb
import time

# =============================================================================
# Extension modules
# =============================================================================
from pyOpt import Optimization
from pyOpt import PSQP
from pyOpt import SLSQP
from pyOpt import CONMIN
from pyOpt import COBYLA
from pyOpt import SOLVOPT
from pyOpt import KSOPT
from pyOpt import NSGA2
from pyOpt import ALGENCAN
import optimize_rtree
from pyswarm import pso

used_solutions = {}

# =============================================================================
# 
# =============================================================================
def get_objective_func(dataset, numElems, numQs, qType):
    """
    dataset  : one of ['real2d', 'syth2d', 'real3d', 'syth3d']
    numElems : the population of the rtree
    numQs    : the number of queries
    qType    : ['within', 'contains', 'covered_by', 'covers', 'disjoint', 
                'intersects', 'knn', 'overlaps']
    """
    ds_num = {
        'real2d' : 1, 'syth2d' : 2, 'real3d' : 3, 'syth3d' : 4
    }[dataset]

    query = {
        'within' : 0, 'contains' : 1, 'covered_by' : 2, 'covers' : 3,
        'disjoint' : 4, 'intersects' : 5, 'knn' : 6, 'overlaps' : 7
    }[qType]

    def rtree_load_and_query(x, *args, **kwargs): 
        """
        x[2] = split type : { 1 : 'lin', 2 : 'qdrt', 3 : 'rstar', 4 : 'bulk' }
        """
        x1 = int(x[1])
        x0 = int(0.1 * x[0] * x1)
        if x0 < 1: x0 = 1

        a = '{}:{}'.format(x0, x1)
        if a in used_solutions:
            print 'reusing solution ({}, {}) for ({}, {})'.format(x0, x1, x[0], x[1])
            return used_solutions[a]

        f = optimize_rtree.objective_function(dataset=ds_num, numElems=numElems, 
                                              numQs=numQs, qType=query, 
                                              minNodes=x0, maxNodes=x1, splitType=1)#x[2])
        #g = [2 * x[0] - x[1]]
        #
        #fail = 0
        #return 0, g, fail
        if f < 0: # TODO: make constraints work
            print 'somebody failed, f returned -1'
            f = 100000
        
        used_solutions[a] = f 
        return f

    return rtree_load_and_query

# =============================================================================
# 
# =============================================================================
def con(x):
    return [2 * x[0] - x[1]]

# =============================================================================
# 
# =============================================================================
def solve_optimization_problem(dataset, rtree_size, query_size, query): 
    """
    creates the objective function and optimization problem
    """
    objfunc = get_objective_func('syth2d', 50000, 10000, 'within')
    ## print objfunc([8, 16, 'lin'])
    #opt_prob = Optimization('R-tree optimization', objfunc)
    #opt_prob.addVar('minNodes', 'c', lower=2., upper=4., value=2.)
    #opt_prob.addVar('maxNodes', 'c', lower=4., upper=8., value=4.)
    ##opt_prob.addVar('split_type', 'd', choices = [1, 2, 3, 4])
    #opt_prob.addObj('minimize latency')
    #opt_prob.addCon('2 * minNodes <= maxNodes','i')
    #print opt_prob
    #
    #psqp = PSQP()
    #psqp.setOption('IPRINT',0)
    #psqp(opt_prob,sens_type='FD')
    #print opt_prob.solution(0)
    lb = [1, 4]
    ub = [5, 128]
    xopt, fopt = pso(objfunc, lb, ub, minstep = 1)
    print xopt
    print fopt
    

# =============================================================================
# 
# =============================================================================
def main():
    """entry point for the application"""
    solve_optimization_problem('real2d', 50000, 10000, 'within')

# =============================================================================
if __name__ == '__main__':
    start = time.time()
    main()
    print 'It took', time.time()-start, 'seconds.'



