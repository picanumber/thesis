import optimize_rtree


print optimize_rtree.objective_function(
    dataset=2, numElems = 50000, numQs=10000, qType=0, minNodes=8, maxNodes=16, splitType=1)
print optimize_rtree.objective_function(
    dataset=2, numElems = 50000, numQs=10000, qType=0, minNodes=5, maxNodes=17, splitType=1)
print optimize_rtree.objective_function(
    dataset=2, numElems = 50000, numQs=10000, qType=0, minNodes=50, maxNodes=120, splitType=1)

