# Author : Anirudh Sivaraman
# Star topology with basestation at the centre.

# Create a simulator object
set ns [ new Simulator ]

# Tracing: DO NOT MOVE THIS BELOW
set trace_file [ open cell-link.tr w ]

# cmd line arguments
unset opt

# Clean up procedures
proc finish { sim_object trace_file } {
  $sim_object flush-trace
  close $trace_file
  exit 0
}

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

# create basestation
set basestation [ $ns node ]

# Create PF scheduler
set num_users [ expr $opt(num_udp) ]
puts "Num users is $num_users, cdma rates available for $opt(cdma_users) users "
assert ( $num_users <= $opt(cdma_users) );
PropFair set num_users_ $num_users
PropFair set slot_duration_  0.00167
set pf_scheduler [ new PropFair ]

# Unique ID
set counter 0

# CBR/UDP clients
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create node
  set udp_client_node($i) [ $ns node ]
  $ns duplex-link $udp_client_node($i) $basestation [ bw_parse $opt(ingress_bw) ] $opt(ingress_latency) $opt(bottleneck_qdisc)

  # Create UDP Agents
  set udp_client($i) [ new Agent/UDP ]
  $ns attach-agent $udp_client_node($i) $udp_client($i)

  # set flow id
  $udp_client($i) set fid_ $counter
  set fid($i) $counter
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
  $ns duplex-link $basestation $udp_server_node($i) [ bw_parse $opt(egress_bw) ]  $opt(egress_latency) $opt(bottleneck_qdisc)

  # Attach queue and link to PF scheduler
  set cell_link [ [ $ns link $basestation $udp_server_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $udp_server_node($i) ] queue ]
 
  # Attach trace_file to queue.
  $ns trace-queue $basestation $udp_server_node($i) $trace_file

  puts "Adding user $fid($i) to PF "
  $cell_queue set blocked_ 1
  $pf_scheduler attach-queue $cell_queue $fid($i)
  $pf_scheduler attach-link  $cell_link  $fid($i)
  # Create Application Sinks
  set udp_server($i) [ new Agent/Null ]
  $ns attach-agent $udp_server_node($i) $udp_server($i)

  # Connect them to their sources
  $ns connect $udp_client($i) $udp_server($i)
}

$pf_scheduler activate-link-scheduler
# Run simulation
$ns at $opt(duration) "finish $ns $trace_file"
$ns run
