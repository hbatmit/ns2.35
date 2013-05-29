# config file for remy simulations
# this one is where all the sources are identical
global opt

# source, sink, and app types
set opt(nsrc) 2;                # number of sources in experiment
set opt(tcp) TCP/Rational
set opt(sink) TCPSink
set opt(cycle_protocols) false
set protocols [list TCP/Rational TCP/Newreno TCP/Linux/cubic TCP/Linux/compound]; # don't put Linux TCPs first on list
set protosinks [list TCPSink TCPSink TCPSink TCPSink]
#set protocols [list TCP/Newreno TCP/Linux/cubic]
#set protosinks [list TCPSink TCPSink/Sack1/DelAck]
#set protocols [list TCP/Newreno TCP/Rational]
#set protocols [list TCP/Newreno/Rational TCP/Linux/cubic ]
#set protosinks [list TCPSink/Sack1 TCPSink/Sack1/DelAck]

# topology parameters
set opt(gw) DropTail;           # queueing at bottleneck
set opt(bneck) 15Mb;             # bottleneck bandwidth (for some topos)
set opt(maxq) 1000;             # max queue length at bottleneck
set opt(rcvwin) 65536
set opt(delay) 74ms;            # total one-way delay in topology
set opt(link) None

# app parameters
set opt(app) FTP/OnOffSender
set opt(pktsize) 1210;          # doesn't include proto headers

# random on-off times for sources
set opt(seed) 0
set opt(onrand) Exponential
set opt(offrand) Exponential
set opt(onavg) 5.0;              # mean on and off time
set opt(offavg) 5.0;              # mean on and off time
set opt(avgbytes) 32000;          # 16 KBytes flows on avg (too low?)
set opt(ontype) "time";           # valid options are "bytes" and "flowcdf"
set opt(reset) "true";              # reset TCP on end of ON period

# simulator parameters
set opt(simtime) 10000.0;        # total simulated time
#set opt(tr) remyout;            # output trace in opt(tr).out
set opt(partialresults) false;   # show partial throughput, delay, and utility?
set opt(verbose) false;          # verbose printing for debugging (esp stats)
set opt(checkinterval) 0.005;        # check stats every 5 ms

# utility and scoring
set opt(alpha) 1.0
set opt(tracewhisk) "none";     # give a connection ID to print for that flow, or give "all"

proc set_access_params { nsrc } {
    global accessdelay
    for {set i 0} {$i < $nsrc} {incr i} {
        set accessdelay($i) 1ms;       # latency of access link
    }
    global accessrate
    for {set i 0} {$i < $nsrc} {incr i} {
        set accessrate($i) 1000Mb;       # speed of access link
    }
}
