import os
import sys
import matplotlib
if os.uname()[0] == 'Darwin':
    matplotlib.use('macosx')
import matplotlib.pyplot as p
import numpy

f = open (sys.argv[1] )
count = 0
lasttime = 0
bws = []
times = []
for l in f:
    fields = l.rstrip().split(" ")
    time = float(fields[len(fields)-1]) # last field
    if time - lasttime > 0.0: 
        times.append(time)
        bws.append(1500.0*8/(time-lasttime)/1000000.0) # in Mbits/s
    lasttime = time

p.figure(1)
p.plot(numpy.array(times), numpy.array(bws))
p.show()
