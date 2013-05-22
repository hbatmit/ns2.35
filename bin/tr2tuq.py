# Takes an ns-2 trace (normal trace) and produces three files:
# time througput
# time qlen
# time link_utilization
# Run as follows:
# trace2tuq.py -f <tracefile> -w <timewindow> -d <dest> -r <bneck router>
# This program assumes a dumbbell topology with n sources connected to
# a gateway, connected over a bottleneck link to the destination.
# All connections go from a source to the same destination.
# Used in congestion control experiments: see tcl/ex/congctrl
# tuq is short for "throughput utilization qlen" and tr refers to "trace".
# ns-2 trace format: http://nsnam.isi.edu/nsnam/index.php/NS-2_Trace_Formats
# 
#Each line has an event that's r, d, e, +, or -, 
# followed by a space-separated string with format 
# %g %d %d %s %d %s %d %d.%d %d.%d %d %d
# where each field is as follows:
# double	Time
# int	(Link-layer) Source Node
# int	(Link-layer) Destination Node
# string	Packet Name
# int	Packet Size
# string	Flags
# int	Flow ID
# int	(Network-layer) Source Address
# int	Source Port
# int	(Network-layer) Destination Address
# int	Destination Port
# int	Sequence Number
# int	Unique Packet ID

import numpy
from optparse import OptionParser

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-f", "--file", type="string", dest="fname", 
                      default = "", help = "trace file")
    parser.add_option("-s", "--src", type="int", dest="src", 
                      default = "0", help = "source node")
    parser.add_option("-d", "--dest", type="int", dest="dest", 
                      default = "3", help = "destination node")
    parser.add_option("-r", "--router", type="int", dest="router", 
                      default = "4", help = "router node")
    parser.add_option("-w", "--window", type="float", dest="window", 
                      default = "0.1", help = "time window")

    (config, args) = parser.parse_args()            

    nsrc = config.dest          # number of sources
    f = open(config.fname, "r")
    nexttime = config.window
    times = []
    tput = []
    totbytes = []
    for i in xrange(nsrc):
        tput.append( [] )
        totbytes.append( [] )
    bytes = [0]*nsrc
    numpkts = [0]*nsrc
    qlen = 0
    qdat = []
    qavg = []

    def gather(qdat):
        if nexttime not in times:
            times.append(nexttime)
            for i in xrange(nsrc):
                tput[i].append(8.0*bytes[i]/(1000000*config.window))
                totbytes[i].append(bytes[i])
                bytes[i] = 0
            qavg.append(numpy.mean(qdat))
            qlen = 0
            qdat = []

    for line in f:
        data = line.split()
        if len(data) != 12: continue
        event = data[0]
        time = float(data[1])
        linksrc = int(data[2])
        linkdst = int(data[3])
        ptype = data[4]
        psize = int(data[5])
        srcaddr = int(data[8].split(".")[0])
        srcport = int(data[8].split(".")[1])
        dstaddr = int(data[9].split(".")[0])
        dstport = int(data[9].split(".")[1])

        while time >= nexttime:
            # gather together all the data for the previous window
            gather(qdat)
            nexttime = nexttime + config.window

        # we know that if we get here time < nexttime
        if (ptype != "tcp") and (ptype != "xcp"): continue
        if event == "r" and linkdst == config.dest:
            # received a packet at the final destination
            numpkts[srcaddr] = numpkts[srcaddr] + 1
            bytes[srcaddr] = bytes[srcaddr] + psize
        elif event == "+" and linksrc == config.router and linkdst == config.dest:
            qlen += 1
            qdat.append(qlen)
        elif event == "-" and linksrc == config.router and linkdst == config.dest:
            qlen -= 1
            qdat.append(qlen)

    gather(qdat)

    j = 0
    for t in times:
        print "%.4f" % t,
        for i in xrange(nsrc):
            print " %.4f %d" % (tput[i][j], totbytes[i][j]),
        print "%.1f" % qavg[j]
        j += 1
