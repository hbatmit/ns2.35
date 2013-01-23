set opt(num_udp)            1
set opt(num_tcp)            1
set opt(ingress_bw)         20Mb
set opt(ingress_latency)    1ms
set opt(egress_bw)          20Mb
set opt(egress_latency)     1ms
set opt(bottleneck_bw)      10Mb
set opt(bottleneck_latency) 20ms
set opt(bottleneck_qdisc)   SFD
set opt(duration)           100
set opt(codel_target)       5ms
set opt(codel_interval)     100ms
set opt(cbr_rate)           10Mb
set opt(iter)               1
set opt(link_type)          deterministic
set opt(sfd_qdisc)          fcfs
set opt(_K)                 0.2
set opt(headroom)           0.05

# TCP parameters
#bdp in packets, based on the nominal rtt
set opt(psize) 1500
set opt(nominal_rtt) [ delay_parse 100ms          ]
set bw          [ bw_parse    $opt(bottleneck_bw) ]
set opt(delack) 0.4
set bdp [expr round( $bw *$opt(nominal_rtt)/(8*$opt(psize)))]

Agent/TCP set window_     [expr $bdp * 16]
Agent/TCP set segsize_    [expr $opt(psize)-40]
Agent/TCP set packetSize_ [expr $opt(psize)-40]
Agent/TCP set windowInit_ 4
Agent/TCP set segsperack_ 1
Agent/TCP set timestamps_ true
Agent/TCP set interval_ $opt(delack)
Agent/TCP/FullTcp set window_     [expr $bdp * 16]
Agent/TCP/FullTcp set segsize_    [expr $opt(psize)-40]
Agent/TCP/FullTcp set packetSize_ [expr $opt(psize)40]
Agent/TCP/FullTcp set windowInit_ 4
Agent/TCP/FullTcp set segsperack_ 1
Agent/TCP/FullTcp set timestamps_ true
Agent/TCP/FullTcp set interval_   $opt(delack)

