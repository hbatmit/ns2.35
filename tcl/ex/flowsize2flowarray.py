import sys
from decimal import *

def proc_flowfile(filename):
    flowcdf = [0]*100001
    f = open(filename)
    for line in f:
        data = line.split()
        cumprob = float(Decimal(data[1]))
        flowsize = int(Decimal(data[0]))
        flowcdf[int(100000*cumprob)] = flowsize

    for i in xrange(len(flowcdf)):
        if i > 0 and flowcdf[i] == 0:
            flowcdf[i] = flowcdf[i-1]

    return flowcdf

## MAIN ##
    
flowcdf = proc_flowfile(sys.argv[1])
print 'global flowcdf'
print 'set flowcdf [list \\'
for i in xrange(len(flowcdf)):
    print flowcdf[i], ' \\'
print ']'
