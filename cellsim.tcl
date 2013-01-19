# Author : Anirudh Sivaraman
# Script to simulate different schedulers on a cellular link

# Create a simulator object
set ns [ new Simulator ]

# Clean up procedures 
proc finish { sim_object trace_file } {
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


# read bandwidths from config file
source configuration.tcl

# get iteration number from cmd line
set iter [ expr [ lindex $argv 0 ] ]
set is_poisson [ lindex $argv 1 ]

# create left_router and right_router
set left_router  [ $ns node ]
set right_router [ $ns node ]

## Set CoDel/sfqCoDel control parameters 
if { $bottleneck_qdisc == "CoDel" } {
  Queue/CoDel    set target_   [ delay_parse $codel_target ]
  Queue/CoDel    set interval_ [ delay_parse $codel_interval ]
}

if { $bottleneck_qdisc == "sfqCoDel" } {
  Queue/sfqCoDel set target_   [ delay_parse $codel_target ]
  Queue/sfqCoDel set interval_ [ delay_parse $codel_interval ]
  Queue/sfqCoDel set d_exp_    0.0
  Queue/sfqCoDel set curq_     0
}

# Set link capacity for SFD
if { $bottleneck_qdisc == "SFD" } {
  Queue/SFD set _capacity [ bw_parse $bottleneck_bw ]
  Queue/SFD set _iter $iter
}

# Set parameters for the DRR queue
if { $bottleneck_qdisc == "DRR" } {
  Simulator instproc get-link { node1 node2 } {
      $self instvar link_
      set id1 [$node1 id]
      set id2 [$node2 id]
      return $link_($id1:$id2)
  }
  set l [$ns get-link $left_router $right_router]
  set q [$l queue]
  $q blimit 25000
  $q quantum 1000
  $q buckets 2
}

set counter 0
# CBR/UDP clients
for { set i 0 } { $i < $num_udp } { incr i } {
  # Create node
  set udp_client_node($i) [ $ns node ]
  $ns duplex-link $udp_client_node($i) $left_router [ bw_parse $ingress_bw ] $ingress_latency DropTail
  
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
  puts "UDP rate $cbr_rate"
  $cbr_client($i) set rate_   [ bw_parse $cbr_rate ]
  $cbr_client($i) set random_     0
  $ns at 0.0 "$cbr_client($i) start"
}

# CBR/UDP servers
for { set i 0 } { $i < $num_udp } { incr i } {
  # Create node
  set udp_server_node($i) [ $ns node ]
  $ns duplex-link $right_router $udp_server_node($i) [ bw_parse $egress_bw ]  $egress_latency DropTail

  # Create Application Sinks
  set udp_server($i) [ new Agent/Null ]
  $ns attach-agent $udp_server_node($i) $udp_server($i)

  # Connect them to their sources
  $ns connect $udp_client($i) $udp_server($i)
}

# FTP/TCP clients
for { set i 0 } { $i < $num_tcp } { incr i } {
  # Create node
  set tcp_client_node($i) [ $ns node ]
  $ns duplex-link $tcp_client_node($i) $left_router [ bw_parse $ingress_bw ] $ingress_latency DropTail

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
for { set i 0 } { $i < $num_tcp } {incr i } {
  # Create node
  set tcp_server_node($i) [ $ns node ]
  $ns duplex-link $right_router $tcp_server_node($i) [ bw_parse $egress_bw ]  $egress_latency DropTail

  # Create server sinks for FTP
  set tcp_server($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_server_node($i) $tcp_server($i)

  # Connect them to their sources
  $ns connect $tcp_client($i) $tcp_server($i)
}

# connect routers by a bottleneck link, with a queue discipline (qdisc)
if { $is_poisson == "poisson"} {
  source link/poisson.tcl
  DelayLink/PoissonLink set _iter $iter
}
$ns duplex-link $left_router $right_router [ bw_parse $bottleneck_bw ] $bottleneck_latency $bottleneck_qdisc

# open a file for tracing bottleneck link alone
set trace_file [ open cellsim.tr w ]
$ns trace-queue $left_router $right_router $trace_file

# Run simulation
$ns at $duration "finish $ns $trace_file"
$ns run
