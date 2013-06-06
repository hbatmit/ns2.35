set opt(num_tcp)            1
set opt(bottleneck_bw)      10Mb
set opt(bottleneck_latency) 20ms
set opt(bottleneck_qdisc)   SFD
set opt(simtime)            100
set opt(codel_target)       5ms
set opt(codel_interval)     100ms
set opt(cbr_rate)           10Mb
set opt(iter)               1
set opt(est_time_constant)  0.2
set opt(headroom)           0.00
set opt(cdma_slot_duration) 0.00167
set opt(ensemble_scheduler) pf
set opt(cdma_ewma_slots)    100
set opt(link_trace)         link.trace
set opt(tcp)                TCP/Sack1
set opt(partialresults) false;    # show partial throughput, delay, and utility scores?
set opt(alpha)              1.0;  # alpha for max-weight scheduling policy
set opt(sub_qdisc)          propfair
set opt(droptype)           "time"
# LoggingApp specific stuff
set opt(onrand) Exponential
set opt(offrand) Exponential
set opt(onavg) 5.0;               # mean on and off time
set opt(offavg) 5.0;              # mean on and off time
set opt(avgbytes) 16000;          # 16 KBytes flows on avg (too low?)
set opt(ontype) "time";           # valid options are "time" and "bytes"
set opt(app) FTP/OnOffSender;     # OnOffSender is our traffic generator
set opt(spike)  "false";          # TODO Ask Hari what this is
set opt(verbose) "false";         # Turn on logging?
set opt(tracing) "false";         # Turn on tracing?
set opt(checkinterval) 0.005;     # Check stats every 5 ms
set opt(reset)   "false";         # reset TCP after every ON period
set opt(flowoffset) 40;           # flow offset

# Accounting for the number of sent and received bytes correctly
set opt(hdrsize) 50
set opt(pktsize) 1450

# TCP parameters
#bdp in packets, based on the nominal rtt
set opt(nominal_rtt) [ delay_parse 100ms          ]
set bw          [ bw_parse    $opt(bottleneck_bw) ]
set opt(delack) 0.4
set bdp [expr round( ($bw *$opt(nominal_rtt))/(8*($opt(pktsize)+$opt(hdrsize))))]

Agent/TCP set window_     [expr $bdp * 16]
Agent/TCP set segsize_    [expr $opt(pktsize) ]
Agent/TCP set packetSize_ [expr $opt(pktsize) ]
Agent/TCP set windowInit_ 4
Agent/TCP set segsperack_ 1
Agent/TCP set timestamps_ true
Agent/TCP set interval_ $opt(delack)
Agent/TCP/FullTcp set window_     [expr $bdp * 16]
Agent/TCP/FullTcp set segsize_    [expr $opt(pktsize)]
Agent/TCP/FullTcp set packetSize_ [expr $opt(pktsize)]
Agent/TCP/FullTcp set windowInit_ 4
Agent/TCP/FullTcp set segsperack_ 1
Agent/TCP/FullTcp set timestamps_ true
Agent/TCP/FullTcp set interval_   $opt(delack)
