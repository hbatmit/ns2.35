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
    puts "Parameter $key"
    assert ( [ info exists opt($key)] );
    puts "Value $val"
    set opt($key) $val
  }
}

Getopt
Usage

# create basestation
set basestation [ $ns node ]

# Determine number of users
set num_users [ expr $opt(num_tcp) + $opt(num_udp)  ]

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

# Create rate generator
set rate_generator [ new EnsembleRateGenerator $opt(link_trace) ]; 
set users_in_trace [ $rate_generator get_users_ ]
puts "Num users is $num_users, users_in_trace $users_in_trace users "
assert ( $num_users == $users_in_trace );

# Set _K and _headroom for SFD
Queue/SFD set _K $opt(_K)
Queue/SFD set _headroom $opt(headroom)

# Unique ID
set counter 0

# Link creation
proc create_link {ns  bw latency sender receiver qdisc} {
  $ns simplex-link $sender $receiver [ bw_parse $bw ]  $latency $qdisc
  $ns simplex-link $receiver $sender [ bw_parse $bw ]  $latency DropTail
}

# Neuter queue
proc neuter_queue {queue} {
  # block queues
  $queue set blocked_ 1
  $queue set unblock_on_resume_ 0

  # Infinite buffer
  $queue set limit_ 1000000000

  # Deactivate forward queue
  $queue deactivate_queue
}

# Add queue and link to ensemble scheduler
proc attach_to_scheduler {scheduler user queue link} {
  puts "Adding user $user to scheduler"
  $scheduler attach-queue $queue $user
  $scheduler attach-link  $link  $user
}

# Setup SFD
proc setup_sfd {queue user link scheduler} {
  $queue user_id $user
  $queue attach-link $link
  $queue attach-sched $scheduler
}

# TCP connections
source setup_tcp_connections.tcl

# UDP connections
source setup_udp_connections.tcl

# Activate scheduler
$ensemble_scheduler activate-link-scheduler

# Activate rate_generator
$rate_generator activate-rate-generator

# Run simulation
$ns at $opt(duration) "finish $ns $trace_file"
$ns run
