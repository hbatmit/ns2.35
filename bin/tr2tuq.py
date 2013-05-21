# Takes an ns-2 trace (normal trace) and produces three files:
# time througput
# time qlen
# time link_utilization
# Run as follows:
# trace2tuq.py -f <tracefile> -w <timewindow> -s <src> -d <dest>
# This program assumes a dumbbell topology with n sources connected to
# a gateway, connected over a bottleneck link to the destination.
# All connections go from a source to the same destination.
# Used in congestion control experiments: see tcl/ex/congctrl
# tuq is short for "throughput utilization qlen" and tr refers to "trace".

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

    def gather():
        for i in xrange(nsrc):
            if nexttime not in times[i]:
                times[i].append(nexttime)
                tput[i].append(8.0*bytes[i]/(1000000*config.window))
            bytes[i] = 0

    nsrc = config.dest          # number of sources
    f = open(config.fname, "r")
    nexttime = config.window
    times = []
    tput = []
    for i in xrange(nsrc):
        times.append([])
        tput.append([])
    bytes = [0]*nsrc
    numpkts = [0]*nsrc
    for line in f:
        data = line.split()
        if len(data) == 12:
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
            gather()
            nexttime = nexttime + config.window

        # we know that if we get here time < nexttime
        if ptype != "tcp": continue
        if event == "r" and linkdst == config.dest:
            # received a packet at the final destination
            numpkts[srcaddr] = numpkts[srcaddr] + 1
            bytes[srcaddr] = bytes[srcaddr] + psize

    gather()
    for i in xrange(nsrc):
        print "SOURCE", i
        print times[i]
        print tput[i]
        print numpkts[i]
