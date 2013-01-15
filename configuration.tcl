set num_udp            1
set num_tcp            1
set ingress_bw         10Mb
set ingress_latency    1ms
set egress_bw          10Mb
set egress_latency     1ms
set bottleneck_bw      10Mb
set bottleneck_latency 1ms
set bottleneck_qdisc   SFD
set duration           100
set codel_target       5ms
set codel_interval     100ms
set cbr_rate           1Mb

# TCP parameters
#bdp in packets, based on the nominal rtt
set psize 1500
set nominal_rtt [ delay_parse 100ms          ]
set bw          [ bw_parse    $bottleneck_bw ]
set bdp [expr round( $bw*$nominal_rtt/(8*$psize))]
set delack 0.4

Agent/TCP set window_     [expr $bdp*16]
Agent/TCP set segsize_    [expr $psize-40]
Agent/TCP set packetSize_ [expr $psize-40]
Agent/TCP set windowInit_ 4
Agent/TCP set segsperack_ 1
Agent/TCP set timestamps_ true
Agent/TCP set interval_ $delack
Agent/TCP/FullTcp set window_     [expr $bdp*16]
Agent/TCP/FullTcp set segsize_    [expr $psize-40]
Agent/TCP/FullTcp set packetSize_ [expr $psize-40]
Agent/TCP/FullTcp set windowInit_ 4
Agent/TCP/FullTcp set segsperack_ 1
Agent/TCP/FullTcp set timestamps_ true
Agent/TCP/FullTcp set interval_   $delack

