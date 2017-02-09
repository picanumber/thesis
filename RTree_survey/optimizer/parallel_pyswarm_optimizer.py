# =============================================================================
# Standard Python modules
# =============================================================================
import os, sys, time
import pdb
import time
import copy
import multiprocessing
from multiprocessing import Process, Queue

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
def print_header(dataset, numElems, numQs, qType, filename):
    """ prints info on the experiment in filename 
    """
    info = 'dataset={}, numElems={}, numQs={}, qType={}\n'.format(
        dataset, numElems, numQs, qType)
    subscr = '=' * (len(info) - 1)

    with open(filename, 'w') as outfile: 
        outfile.write('\n\n\t*** R-tree optimization with pyswarm ***\n\n')
        outfile.write(subscr + '\n')
        outfile.write(info)
        outfile.write(subscr + '\n\n')

# =============================================================================
def rtree_load_and_query(x, *args, **kwargs): 
        """ objective function
        """
        x1 = get_max_nodes(x[1])
        x0 = get_min_nodes(x[0], x1)
        
        opt = kwargs['opt_task']

        a = '{}:{}'.format(x0, x1)
        if a in opt.used_solutions:
            print 'reusing solution ({}, {}) for ({}, {})'.format(x0, x1, x[0], x[1])
            return opt.used_solutions[a]

        # 2 lines below only for debug purposes
        #f = x1**4 - 2*x0*x1**2 + x0**2 + x1**2 - 2*x1 + 5
        #return f
        f = optimize_rtree.objective_function(dataset=opt.ds_num, numElems=opt.numElems, 
                                              numQs=opt.numQs, qType=opt.query, minNodes=x0, 
                                              maxNodes=x1, splitType=opt.split_algo)
        
        if f < 0: # TODO: make constraints work
            print 'somebody failed, f returned -1'
            f = 100000
        
        opt.used_solutions[a] = f 
        return f

# =============================================================================
class optimization_task(multiprocessing.Process):
    def __init__(self, id, dataset, numElems, numQs, qType, queue):
        """
        dataset  : one of ['real2d', 'syth2d', 'real3d', 'syth3d']
        numElems : the population of the rtree
        numQs    : the number of queries
        qType    : ['within', 'contains', 'covered_by', 'covers', 'disjoint', 
                    'intersects', 'knn', 'overlaps']
        """
        # mandated by multiprocessing
        super(optimization_task, self).__init__()
        self.id = id

        # implementation : set up the optimization task
        self.used_solutions = {}
        self.ds_num         = { 'real2d' : 1, 'syth2d' : 2, 'real3d' : 3, 'syth3d' : 4 }[dataset]
        self.numElems       = numElems
        self.numQs          = numQs
        self.query          = {
            'within' : 0, 'contains' : 1, 'covered_by' : 2, 'covers' : 3,
            'disjoint' : 4, 'intersects' : 5, 'knn' : 6, 'overlaps' : 7
        }[qType]

        # Process needs a Queue to receive the results
        self.q = queue

    def set_split(self, split): 
        """ execution policy for the optimization task
        """
        self.split_type = split
        self.split_algo = { 'lin' : 1, 'qdrt' : 2, 'rstar' : 3, 'bulk' : 4 }[split]

    def run(self): 
        """ runs pyswarm
        """
        lb = [1, 4]
        ub = [5, 128]
        
        xopt, fopt = pso(rtree_load_and_query, lb, ub, minstep = 1, kwargs = {'opt_task': self})
        
        x1 = get_max_nodes(xopt[1])
        x0 = get_min_nodes(xopt[0], x1)
        rec = 'optimum latency = {} | nodes = ({}, {}) | split = {}'.format(
            fopt, x0, x1, self.split_type)
        self.q.put(rec) # to export results in m-threaded

# =============================================================================
def multi_threaded(dataset, numElems, numQs, qType, filename):
    """ calls the optimization tasks in parallel """ 
    q = Queue()
    processes = []
    
    for i, split in zip(range(4), ['lin', 'qdrt', 'rstar', 'bulk']): 
        processes.append(optimization_task(i, dataset, numElems, numQs, qType, q))
        processes[-1].set_split(split)

    [p.start() for p in processes]    
    
    print_header(dataset, numElems, numQs, qType, filename)
    # append results
    for p in processes:
        p.join()
        with open(filename, 'a') as outfile:
            outfile.write(q.get() + '\n')
        
    return 0

# =============================================================================
def main():
    # TODO: command line arguments should control these
    multi_threaded('syth2d', 50000, 10000, 'within', 'pyswarm_multithreaded.txt'); 

# =============================================================================
if __name__ == '__main__':
    start = time.time()
    main()
    secs = time.time()-start
    print 'Optimization duration: ', (secs / 60.), ' minutes.'



