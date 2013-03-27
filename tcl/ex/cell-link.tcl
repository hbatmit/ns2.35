# Author : Anirudh Sivaraman
# Star topology with basestation at the centre.

# Create a simulator object
set ns [ new Simulator ]

# Tracing: DO NOT MOVE THIS BELOW
set trace_file [ open cell-link.tr w ]

# required for reading cmd line arguments
unset opt

# Clean up procedures
proc finish { sim_object trace_file } {
  $sim_object flush-trace
  close $trace_file
  exit 0
}

# read default constants from config file
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
    assert ( [ info exists opt($key)] );
    set opt($key) $val
  }
}

Getopt
Usage

# create basestation
set basestation [ $ns node ]

# Determine number of users
set num_users [ expr $opt(num_tcp) + $opt(num_udp)  ]
puts "Num users is $num_users, cdma rates available for $opt(cdma_users) users "
assert ( $num_users <= $opt(cdma_users) );

# Change parameters for both schedulers
FcfsScheduler set num_users_ $num_users
PFScheduler   set num_users_ $num_users
PFScheduler   set slot_duration_  $opt(cdma_slot_duration)
PFScheduler   set ewma_slots_ $opt(cdma_ewma_slots)

# Pick appropriate ensemble_scheduler
if { $opt(ensemble_scheduler) == "pf" } {
  set ensemble_scheduler [ new PFScheduler ]
} elseif { $opt(ensemble_scheduler) == "fcfs" } {
  set ensemble_scheduler [ new FcfsScheduler ]
}

# Set _K and _headroom for SFD
Queue/SFD set _K $opt(_K)
Queue/SFD set _headroom $opt(headroom)

# Unique ID
set counter 0

# TCP servers
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create TCP Agents
  set tcp_server($i) [ new Agent/TCP/Linux ]
  $tcp_server($i) select_ca $opt(congestion_control)
  $ns attach-agent $basestation $tcp_server($i)

  # set flow id
  $tcp_server($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Generate FTP traffic on TCP Agent
  set ftp_server($i) [ new Application/FTP ]
  $ftp_server($i) attach-agent $tcp_server($i)
  $ns at 0.0 "$ftp_server($i) start"
}

# TCP clients
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create node corresponding to mobile user
  set tcp_client_node($i) [ $ns node ]
  $ns simplex-link $basestation $tcp_client_node($i) [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) $opt(bottleneck_qdisc)
  $ns simplex-link $tcp_client_node($i) $basestation [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) DropTail

  # Get handles to link and queue
  set cell_link [ [ $ns link $basestation $tcp_client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $tcp_client_node($i) ] queue ]

  # All the non-standard queue neutering:
  # block queues
  $cell_queue set blocked_ 1
  $cell_queue set unblock_on_resume_ 0

  # Infinite buffer
  $cell_queue set limit_ 1000000000

  # Deactivate forward queue
  $cell_queue deactivate_queue

  # Set user_id and other stuff for SFD
  if { $opt(bottleneck_qdisc) == "SFD" } {
    $cell_queue user_id $i
    $cell_queue attach-link $cell_link
    $cell_queue attach-sched $ensemble_scheduler
  }

  # Attach trace_file to queue.
  $ns trace-queue $basestation $tcp_client_node($i) $trace_file

  # Attach queue and link to ensemble_scheduler
  puts "Adding user $fid($i) to scheduler"
  $ensemble_scheduler attach-queue $cell_queue $fid($i)
  $ensemble_scheduler attach-link  $cell_link  $fid($i)

  # Create tcp sinks
  set tcp_client($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_client_node($i) $tcp_client($i)

  # Connect them to their sources
  $ns connect $tcp_server($i) $tcp_client($i)
}

# CBR/UDP server
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create UDP Agents
  set udp_server($i) [ new Agent/UDP ]
  $ns attach-agent $basestation $udp_server($i)

  # set flow id
  $udp_server($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Generate CBR traffic on UDP Agent
  set cbr_server($i) [ new Application/Traffic/CBR ]
  $cbr_server($i) attach-agent $udp_server($i)

  # Set packetSize_ and related parameters.
  $cbr_server($i) set packetSize_ 1000
  puts "UDP rate $opt(cbr_rate)"
  $cbr_server($i) set rate_   [ bw_parse $opt(cbr_rate) ]
  $cbr_server($i) set random_     0
  $ns at 0.0 "$cbr_server($i) start"
}

# CBR/UDP clients
for { set i 0 } { $i < $opt(num_udp) } { incr i } {
  # Create node corresponding to mobile user
  set udp_client_node($i) [ $ns node ]
  $ns simplex-link $basestation $udp_client_node($i) [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) $opt(bottleneck_qdisc)
  $ns simplex-link $udp_client_node($i) $basestation [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) DropTail

  # Get references to cell_link and cell_queue
  set cell_link [ [ $ns link $basestation $udp_client_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $udp_client_node($i) ] queue ]

  # All the non-standard queue neutering:
  # block queues
  $cell_queue set blocked_ 1
  $cell_queue set unblock_on_resume_ 0

  # Infinite buffer
  $cell_queue set limit_ 1000000000

  # Deactivate forward queue
  $cell_queue deactivate_queue

  # Set user id and other stuff for SFD
  if { $opt(bottleneck_qdisc) == "SFD" } {
    $cell_queue user_id $i
    $cell_queue attach-link $cell_link
    $cell_queue attach-sched $ensemble_scheduler
  }

  # Attach trace_file to queue.
  $ns trace-queue $basestation $udp_client_node($i) $trace_file

  # Attach queue and link to ensemble scheduler
  puts "Adding user $fid($i) to scheduler "
  $ensemble_scheduler attach-queue $cell_queue $fid($i)
  $ensemble_scheduler attach-link  $cell_link  $fid($i)

  # Create Application Sinks
  set udp_client($i) [ new Agent/Null ]
  $ns attach-agent $udp_client_node($i) $udp_client($i)

  # Connect them to their sources
  $ns connect $udp_server($i) $udp_client($i)
}

# Activate scheduler
$ensemble_scheduler activate-link-scheduler
# Run simulation
$ns at $opt(duration) "finish $ns $trace_file"
$ns run
