# =============================================================================
# Standard Python modules
# =============================================================================
import os, sys, time
import pdb
import time
import copy
import itertools
import pickle
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
def to_k_format(num): 
    """ 
    Returns an easy to read numerical representation, e.g.
    50000 -> 50K
    5     -> 5
    """
    if num >= 1e06: return "{}M".format(num / 1e06)
    if num >= 1e03: return "{}K".format(num / 1e03)
    return num

# =============================================================================
def print_header(dataset, numElems, numQs, qType, filename, minutes):
    """ prints info on the experiment in filename 
    """
    info = 'dataset={}, numElems={}, numQs={}, qType={}\n'.format(
        dataset, to_k_format(numElems), to_k_format(numQs), qType)
    subscr = '=' * (len(info) - 1)

    with open(filename, 'a') as outfile: 
        outfile.write(
            '\n\n\t*** R-tree optimization with pyswarm ({} minutes) ***\n\n'.
            format(minutes))
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

        # implementation : write down the optimization parameters
        self.data_set = dataset
        self.tree_size = numElems
        self.q_size = numQs
        self.q_type = qType

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
        
        optimize_rtree.clear_problem_space()
        xopt, fopt = pso(rtree_load_and_query, lb, ub, minstep = 1, 
                         minfunc = 1, maxiter = 10, swarmsize = 50, kwargs = {'opt_task': self})
        
        x1 = get_max_nodes(xopt[1])
        x0 = get_min_nodes(xopt[0], x1)
        rec = 'optimum latency = {} | nodes = ({}, {}) | split = {}'.format(
            fopt, x0, x1, self.split_type)

        # log info on the search space
        sp_file = "search_spaces/{}_{}_{}_{}_{}.txt".format(
            self.split_type, self.q_type, self.data_set, 
            to_k_format(self.tree_size), to_k_format(self.q_size))

        with open(sp_file, 'wb') as outf: 
            pickle.dump(self.used_solutions, outf)

        # export results in m-threaded
        self.q.put(rec) 

# =============================================================================
def multi_threaded(dataset, numElems, numQs, qType, filename):
    """ 
    calls the optimization tasks in parallel, e.g.
    multi_threaded('real2d', 15000, 1500, 'overlaps', 'pyswarm_multithreaded.txt')
    """ 
    start = time.time()
    
    # 1. 
    q = Queue()
    processes = [optimization_task(i, dataset, numElems, numQs, qType, q) for i in range(4)]
    [p.set_split(split) for p, split in zip(processes, ['lin', 'qdrt', 'rstar', 'bulk'])]

    # 2. 
    [p.start() for p in processes]    
    [p.join() for p in processes]

    secs = time.time() - start

    # 3. 
    print_header(dataset, numElems, numQs, qType, filename, (secs // 60))
    with open(filename, 'a') as outfile:
        lines = {}
        while not q.empty(): 
            words = q.get()
            key = words.split('|')[-1].strip().split('=')[-1].strip()
            lines[key] = words
        for elem in lines: 
            outfile.write(lines[elem] + '\n')

    return 0

# =============================================================================
def run_optimization_setups(datasets, tree_sizes, query_sizes, query_types, fout): 
    """ runs the cartesian product of the combinations given
    """
    for elem in itertools.product(datasets, tree_sizes, query_sizes, query_types): 
        multi_threaded(*elem, filename=fout)

    return 

# =============================================================================
def main():
    """ entry point for the application
    """
    d_set = ['real2d', 'syth2d']
    qType = ['within', 'overlaps']
        
    run_optimization_setups(
        d_set, [50000], [25000, 50000, 75000, 100000], qType, 
        'optimize_fixed_tree_size.txt')

    run_optimization_setups(
        d_set, [25000, 75000, 100000], [50000], qType, 
        'optimize_fixed_query_size.txt')
    
# =============================================================================
if __name__ == '__main__':
    raw_input("Press Enter to proceed...")
    main()



