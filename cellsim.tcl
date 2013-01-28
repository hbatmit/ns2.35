# Author : Anirudh Sivaraman
# Script to simulate different schedulers on a cellular link

# Create a simulator object
set ns [ new Simulator ]

unset opt
# Clean up procedures 
proc finish { sim_object trace_file } {
  global ns left_router right_router
  [ [ $ns link $left_router $right_router ] link ] total
  $sim_object flush-trace
  close $trace_file
  exit 0
}

# UDP topology :
#
#  udp_client_node(0)  --                                    ---udp_server_node(0)
#                        \                                  /
#  udp_client_node(1)----left_router-----------right_router----udp_server_node(1)
#             .          /                                  \        .
#             .         /                                    \       .
#             .        /                                      \      .
#  udp_client_node(n)                                          udp_server_node(n)

# TCP topology :
#
#  tcp_client_node(0)  --                                    ---tcp_server_node(0)
#                        \                                  /
#  tcp_client_node(1)----left_router-----------right_router----tcp_server_node(1)
#             .          /                                  \        .
#             .         /                                    \       .
#             .        /                                      \      .
#  tcp_client_node(n)                                          tcp_server_node(n)


# read default bandwidths from config file
source configuration.tcl

# Override them using the command line if desired

proc Usage {} {
  global opt argv0
  foreach name [ array names opt ] {
    puts -nonewline " \[-"
    puts -nonewline [ format "%20s" $name ]
    puts -nonewline " :\t"
    puts -nonewline [ format "%15.15s"  $opt($name) ]
    puts -nonewline "\]\n"
  }
}

proc Getopt {} {
  global opt argc argv argv0
  if {$argc == 0} {
    puts "Usage : $argv0 \n"
    Usage
    exit 1
  }
  for {set i 0} {$i < $argc} {incr i} {
    set key [lindex $argv $i]
    if ![string match {-*} $key] continue
    set key [string range $key 1 end]
    set val [lindex $argv [incr i]]
    set opt($key) $val
  }
}

Getopt
Usage

# create left_router and right_router
set left_router  [ $ns node ]
set right_router [ $ns node ]

## Set CoDel/sfqCoDel control parameters 
if { $opt(bottleneck_qdisc) == "CoDel" } {
  Queue/CoDel    set target_   [ delay_parse $opt(codel_target) ]
  Queue/CoDel    set interval_ [ delay_parse $opt(codel_interval) ]
}

if { $opt(bottleneck_qdisc) == "sfqCoDel" } {
  Queue/sfqCoDel set target_   [ delay_parse $opt(codel_target) ]
  Queue/sfqCoDel set interval_ [ delay_parse $opt(codel_interval) ]
  Queue/sfqCoDel set d_exp_    0.0
  Queue/sfqCoDel set curq_     0
}

# Set link capacity for SFD
if { $opt(bottleneck_qdisc) == "SFD" } {
  Queue/SFD set _capacity [ bw_parse $opt(bottleneck_bw) ]
  Queue/SFD set _iter $opt(iter)
  Queue/SFD set _qdisc [ expr [ string equal $opt(sfd_qdisc) "fcfs" ] == 1 ? 0 : 1 ]
  Queue/SFD set _K $opt(_K)
  Queue/SFD set _headroom $opt(headroom)
}

# DRR defaults for simulation
Queue/DRR set buckets_ 100
Queue/DRR set blimit_ 25000
Queue/DRR set quantum_ 1000
Queue/DRR set mask_ 0

set counter 0
# CBR/UDP clients
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create node
  set udp_client_node($i) [ $ns node ]
  $ns duplex-link $udp_client_node($i) $left_router [ bw_parse $opt(ingress_bw) ] $opt(ingress_latency) DropTail
  
  # Create UDP Agents 
  set udp_client($i) [ new Agent/UDP ]
  $ns attach-agent $udp_client_node($i) $udp_client($i)

  # set flow id
  $udp_client($i) set fid_ $counter
  set counter [ incr counter ]

  # Generate CBR traffic on UDP Agent
  set cbr_client($i) [ new Application/Traffic/CBR ]
  $cbr_client($i) attach-agent $udp_client($i)

  # Set packetSize_ and related parameters.
  $cbr_client($i) set packetSize_ 1000
  puts "UDP rate $opt(cbr_rate)"
  $cbr_client($i) set rate_   [ bw_parse $opt(cbr_rate) ]
  $cbr_client($i) set random_     0
  $ns at 0.0 "$cbr_client($i) start"
}

# CBR/UDP servers
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create node
  set udp_server_node($i) [ $ns node ]
  $ns duplex-link $right_router $udp_server_node($i) [ bw_parse $opt(egress_bw) ]  $opt(egress_latency) DropTail

  # Create Application Sinks
  set udp_server($i) [ new Agent/Null ]
  $ns attach-agent $udp_server_node($i) $udp_server($i)

  # Connect them to their sources
  $ns connect $udp_client($i) $udp_server($i)
}

# FTP/TCP clients
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create node
  set tcp_client_node($i) [ $ns node ]
  $ns duplex-link $tcp_client_node($i) $left_router [ bw_parse $opt(ingress_bw) ] $opt(ingress_latency) DropTail

  # Create TCP Agents
  set tcp_client($i) [ new Agent/TCP/Linux ]
  $tcp_client($i) select_ca cubic
  $ns attach-agent $tcp_client_node($i) $tcp_client($i)

  # set flow id
  $tcp_client($i) set fid_ $counter
  set counter [ incr counter ]

  # Generate FTP traffic on TCP Agent
  set ftp_client($i) [ new Application/FTP ]
  $ftp_client($i) attach-agent $tcp_client($i)
  $ns at 0.0 "$ftp_client($i) start"
}

# FTP/TCP traffic servers
for { set i 0 } { $i < $opt(num_tcp) } {incr i } {
  # Create node
  set tcp_server_node($i) [ $ns node ]
  $ns duplex-link $right_router $tcp_server_node($i) [ bw_parse $opt(egress_bw) ]  $opt(egress_latency) DropTail

  # Create server sinks for FTP
  set tcp_server($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_server_node($i) $tcp_server($i)

  # Connect them to their sources
  $ns connect $tcp_client($i) $tcp_server($i)
}

# connect routers by a bottleneck link, with a queue discipline (qdisc)
if { $opt(link_type) == "poisson"} {
  source link/poisson.tcl
  DelayLink/PoissonLink set _iter $opt(iter)
  $ns duplex-link $left_router $right_router [ bw_parse $opt(bottleneck_bw) ] $opt(bottleneck_latency) $opt(bottleneck_qdisc)

} elseif { $opt(link_type) == "trace"} {
  source link/trace.tcl
  $ns simplex-link $left_router $right_router [ bw_parse $opt(bottleneck_bw) ] $opt(bottleneck_latency) $opt(bottleneck_qdisc)
  [ [ $ns link $left_router $right_router ] link ] trace-file "uplink.pps"
  $ns simplex-link $right_router $left_router [ bw_parse $opt(bottleneck_bw) ] $opt(bottleneck_latency) $opt(bottleneck_qdisc)
  [ [ $ns link $right_router $left_router ] link ] trace-file "downlink.pps"

} elseif { $opt(link_type) == "brownian" } {
  source link/brownian.tcl
  DelayLink/BrownianLink set _iter $opt(iter)
  # Min rate 80 MTU sized packets per second or ~ 1mbps
  DelayLink/BrownianLink set _min_rate 100
  # Max rate 800 MTU sized packets per second or ~ 10mbps
  DelayLink/BrownianLink set _max_rate 100.1
  DelayLink/BrownianLink set _duration $opt(duration)
  $ns duplex-link $left_router $right_router [ bw_parse $opt(bottleneck_bw) ] $opt(bottleneck_latency) $opt(bottleneck_qdisc)

} elseif { $opt(link_type) == "deterministic" } {
  puts "Link type determinstic"
  $ns duplex-link $left_router $right_router [ bw_parse $opt(bottleneck_bw) ] $opt(bottleneck_latency) $opt(bottleneck_qdisc)

} else {
  puts "Invalid link type"
  exit 5

}


# open a file for tracing bottleneck link alone
set trace_file [ open cellsim.tr w ]
$ns trace-queue $left_router $right_router $trace_file

# Run simulation
$ns at $opt(duration) "finish $ns $trace_file"
$ns run
