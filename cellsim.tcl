# Author : Anirudh Sivaraman
# Script to simulate different schedulers on a cellular link

# Create a simulator object
set ns [ new Simulator ]

# open a file for tracing
set trace_file [ open cellsim.tr w ]
$ns trace-all $trace_file

# Clean up procedures 
proc finish { sim_object trace_file } {
  $sim_object flush-trace
  close $trace_file 
  exit 0
}

# Generic topology :
#
#  client_node(0) --                                    ---server_node(0)
#                   \                                  /
#  client_node(1)----left_router-----------right_router----server_node(1)
#        .          /                                  \        .
#        .         /                                    \       .
#        .        /                                      \      .
#  client_node(n)                                          server_node(m)

# read bandwidths from config file
source configuration.tcl

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

if { $bottleneck_qdisc == "SFD" } {
  Queue/SFD set _capacity [ bw_parse $bottleneck_bw ]
}

# connect them by a bottleneck link, with a queue discipline (qdisc)
$ns duplex-link $left_router $right_router $bottleneck_bw $bottleneck_latency $bottleneck_qdisc

# Create number of required client nodes
for { set i 0 } { $i < $num_clients } { incr i } {
  set client_node($i) [ $ns node ]
  $ns duplex-link $client_node($i) $left_router $ingress_bw $ingress_latency DropTail
}

# Create number of required server nodes
for { set i 0 } { $i < $num_servers } { incr i } {
  set server_node($i) [ $ns node ]
  $ns duplex-link $right_router $server_node($i) $egress_bw  $egress_latency DropTail
}

# Create traffic sources on client
for { set i 0 } { $i < $num_clients } { incr i } {
  
  # Create UDP Agents 
  set client_udp($i) [ new Agent/UDP ]
  $ns attach-agent $client_node($i) $client_udp($i)

  # Generate CBR traffic on UDP Agent
  set client_cbr($i) [ new Application/Traffic/CBR ]
  $client_cbr($i) attach-agent $client_udp($i)

  # Set packetSize_ and related parameters.
  $client_cbr($i) set packetSize_ 1000
  set rate [ expr ( $i + 1 ) * 5 ]
  append rate "Mb"
  puts $rate
  $client_cbr($i) set rate_   $rate
  $client_cbr($i) set random_     0
  $ns at 0.0 "$client_cbr($i) start"
}

# Create traffic receivers on server
for { set i 0 } { $i < $num_servers } { incr i } {

  # Create Application Sinks
  set server_sink($i) [ new Agent/Null ]
  $ns attach-agent $server_node($i) $server_sink($i)

  # Connect them to their sources
  $ns connect $client_udp($i) $server_sink($i)
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

# Run simulation
$ns at $duration "finish $ns $trace_file"
$ns run
