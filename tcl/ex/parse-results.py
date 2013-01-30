import sys
import os
import glob
import numpy
import matplotlib
if os.uname()[0] == 'Darwin':
    matplotlib.use('macosx')
import matplotlib.pyplot as p

def proc_one_file(filename):
    expt_params = filename.split(".")
    for field in expt_params:
        if (field.find("nconn") != -1):
            numconns = int(field.lstrip('nconn'))
                           
    f = open(filename, "rb")
    throughput = []
    delay = []
    util = []
    for line in f:
        data =  line.split()
        if data[0] != "FINAL":
            continue
        throughput.append(float(data[3]))
        delay.append(float(data[4]))
        util.append(float(data[6]))
        
    return numconns, numpy.mean(throughput), numpy.std(throughput), numpy.mean(delay), numpy.std(delay), numpy.mean(util), numpy.std(util)


## MAIN ##

fnames = glob.glob(sys.argv[1]+"/*simtime*")
throughput = {}
delay = {}
for filename in fnames:
    proto = filename.split("/")[1].split(".")[0]
    n, th, thstd, d, dstd, u, ustd =  proc_one_file(filename)
    print "%d %s %.2f %.1f %.1f %.1f %.2g %.1f" % ( n, proto, th, thstd, d, dstd, u, ustd)
    if proto in throughput:
        throughput[proto].append(th)
        delay[proto].append(d)
    else:
        throughput[proto] = [th]
        delay[proto] = [d]

#
#colors = [ ('+', 'red'), ('o', 'blue'), ('*', 'green'), ('-', 'magenta'), ('.', 'black'), ('=', 'cyan'), ('+', 'blue') ]
#
#i = 0
#for proto in throughput:
#    p.figure(1)
#    x = numpy.array(throughput[proto])
#    y = numpy.array(delay[proto])
#    p.scatter(x, y, s=500)
#    i = i + 1
#    
#p.show()
