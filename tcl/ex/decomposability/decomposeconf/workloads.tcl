# config file for remy simulations
global opt

# source, sink, and app types
set opt(tcp) TCP/Rational
set opt(sink) TCPSink

# AQM details
set opt(gw) DropTail;            # queueing at bottleneck
set opt(rcvwin) 2000000

# app parameters
set opt(app) FTP/OnOffSender
set opt(pktsize) 1200;           # doesn't include proto headers

# random on-off times for sources
set opt(run) 1
set opt(onrand) Exponential
set opt(offrand) Exponential
set opt(onavg) 5.0;              # mean on and off time
set opt(offavg) 0.2;             # mean on and off time
set opt(ontype) "bytes";         # valid options: "time", "bytes", and "flowcdf"

# simulator parameters
set opt(simtime) 100.0;          # total simulated time
#set opt(tr) remyout;            # output trace in opt(tr).out
set opt(partialresults) false;   # show partial throughput, delay, and utility?
set opt(verbose) false;          # verbose printing for debugging (esp stats)
set opt(checkinterval) 0.0001;    # check stats every 5 ms

# utility and scoring
set opt(alpha) 1.0
set opt(tracewhisk) "none";      # give a connection ID to print for that flow, or give "all"

# tcp details
Agent/TCP set tcpTick_ .0001
Agent/TCP set timestamps_ true
set opt(hdrsize) 50
set opt(flowoffset) 40
