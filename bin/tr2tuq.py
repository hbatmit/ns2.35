# Takes an ns-2 trace (normal trace) and produces three files:
# time througput
# time qlen
# time link_utilization
# Run as follows:
# trace2tuq.py -d <dest> -q <bottleneck_queue_node>
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

    f = open(config.fname, "r")
    nexttime = config.window
    times = []
    tput = []
    bytes = 0
    for line in f:
        data = line.split()
        if len(data) == 12:
            event = data[0]
            time = float(data[1])
            linksrc = data[2]
            linkdst = data[3]
            psize = int(data[5])
            srcaddr = int(data[8].split(".")[0])
            srcport = int(data[8].split(".")[1])
            dstaddr = int(data[9].split(".")[0])
            dstport = int(data[9].split(".")[1])
            

        if time < nexttime:
            if event == "r" and srcaddr == config.src and dstaddr == config.dest:
                # received a packet at the final destination
                bytes = bytes + psize
        else:
            if event == "r" and dstaddr == config.dest:
                # received a packet at the final destination
                # got one more throughput sample
                times.append(nexttime)
                tput.append(1.0*bytes/config.window)
                nexttime = nexttime + config.window
                bytes = psize

    if nexttime not in times:
        times.append(nexttime)
        tput.append(1.0*bytes/config.window)
        nexttime = nexttime + config.window

    print times
    print tput

