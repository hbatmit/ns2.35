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
set num_users [ expr $opt(num_tcp) ]
puts "Num users is $num_users, cdma rates available for $opt(cdma_users) users "
assert ( $num_users <= $opt(cdma_users) );
PFScheduler set num_users_ $num_users
PFScheduler set slot_duration_  0.00167
set pf_scheduler [ new PFScheduler ]

# Unique ID
set counter 0

# CBR/UDP clients
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create UDP Agents
  set tcp_client($i) [ new Agent/TCP ]
  $ns attach-agent $basestation $tcp_client($i)

  # set flow id
  $tcp_client($i) set fid_ $counter
  set fid($i) $counter
  set counter [ incr counter ]

  # Generate FTP traffic on TCP Agent
  set ftp_client($i) [ new Application/FTP ]
  $ftp_client($i) attach-agent $tcp_client($i)
  $ns at 0.0 "$ftp_client($i) start"
}

# CBR/UDP servers
for { set i 0 } { $i < $opt(num_tcp) } { incr i } {
  # Create node
  set tcp_server_node($i) [ $ns node ]
  $ns simplex-link $basestation $tcp_server_node($i) [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) $opt(bottleneck_qdisc)
  $ns simplex-link $tcp_server_node($i) $basestation [ bw_parse $opt(bottleneck_bw) ]  $opt(bottleneck_latency) DropTail

  # Attach queue and link to PF scheduler
  set cell_link [ [ $ns link $basestation $tcp_server_node($i) ] link ]
  set cell_queue [ [ $ns link $basestation $tcp_server_node($i) ] queue ]

  # All the non-standard queue neutering:
  # block queues
  $cell_queue set blocked_ 1
  $cell_queue set unblock_on_resume_ 0

  # Infinite buffer
  $cell_queue set limit_ 1000000000

  # Deactivate forward queue
  $cell_queue deactivate_queue

  # Set user id
  if { $opt(bottleneck_qdisc) == "SFD" } {
    $cell_queue user_id $i
    $cell_queue attach-link $cell_link
    $cell_queue attach-sched $pf_scheduler
  }

  # Attach trace_file to queue.
  $ns trace-queue $basestation $tcp_server_node($i) $trace_file

  puts "Adding user $fid($i) to PF "
  $pf_scheduler attach-queue $cell_queue $fid($i)
  $pf_scheduler attach-link  $cell_link  $fid($i)

  # Crate tcp sinks
  set tcp_server($i) [ new Agent/TCPSink/Sack1 ]
  $ns attach-agent $tcp_server_node($i) $tcp_server($i)

  # Connect them to their sources
  $ns connect $tcp_client($i) $tcp_server($i)
}

# Activate PF scheduler
$pf_scheduler activate-link-scheduler
# Run simulation
$ns at $opt(duration) "finish $ns $trace_file"
$ns run
