# config file for remy simulations
# this one is where all the sources are identical
global opt

# source, sink, and app types
set opt(nsrc) 2;                # number of sources in experiment
set opt(tcp) TCP/Reno
set opt(sink) TCPSink
set opt(app) FTP
set opt(pktsize) 1210
set opt(rcvwin) 16384

# topology parameters
set opt(gw) DropTail;           # queueing at bottleneck
set opt(bneck) 10Mb;             # bottleneck bandwidth (for some topos)
set opt(maxq) 1000;             # max queue length at bottleneck
set opt(delay) 49ms;            # total one-way delay in topology
global accessdelay
for {set i 0} {$i < $opt(nsrc)} {incr i} {
    set accessdelay($i) [expr 25*$i + 1]ms;       # latency of access link
}
global accessrate
for {set i 0} {$i < $opt(nsrc)} {incr i} {
    set accessrate($i) 1000Mb;       # latency of access link
}

# random on-off times for sources
set opt(seed) 0
set opt(onrand) Exponential
set opt(offrand) Exponential
set opt(onavg) 5.0;              # mean on and off time
set opt(offavg) 5.0;              # mean on and off time
set opt(avgbytes) 16000;          # 16 KBytes flows on avg (too low?)
set opt(ontype) "time";           # valid options are "time" and "bytes"

# simulator parameters
set opt(simtime) 100.0;        # total simulated time
set opt(tr) remyout;            # output trace in opt(tr).out
set opt(partialresults) false;   # show partial throughput, delay, and utility scores?

# utility and scoring
set opt(alpha) 1.0
set opt(tracewhisk) "none";     # give a connection ID to print for that flow, or give "all"

