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
lpfbws = []
times = []
alpha = 0.1
for l in f:
    fields = l.lstrip().rstrip().split(" ")
    if fields[0] != "DIR:" or len(fields) < 8:
        continue
    time = float(fields[len(fields)-1]) # last field

    if time - lasttime > 0.0: 
        times.append(time)
        bw = (1500.0*8/(time-lasttime)/1000000.0) # in Mbits/s
        bws.append(bw)
        if len(lpfbws) == 0:
            lpfbws = [bw]
        else:
            lpfbws.append(alpha*bw + (1.0-alpha)*lpfbws[len(lpfbws)-1])
    lasttime = time

p.figure(1)
#p.plot(numpy.array(times), numpy.array(bws))
p.plot(numpy.array(times), numpy.array(lpfbws))
p.show()
