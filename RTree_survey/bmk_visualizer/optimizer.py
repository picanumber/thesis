import optimize_rtree





def main():
    ret = optimize_rtree.objective_function(1, "a", "aa", "a")
    print('function returned {}'.format(ret))
    #print(dir(optimize_rtree))
    #print(optimize_rtree.__doc__)


if __name__ == '__main__':
    main()