# Author : Anirudh Sivaraman
# Star topology with basestation at the centre.

#!/bin/sh
# the next line finds ns \
nshome=`dirname $0`; [ ! -x $nshome/ns ] && [ -x ../../../ns ] && nshome=../../..
# the next line starts ns \
export nshome; exec $nshome/ns "$0" "$@"

if [info exists env(nshome)] {
	set nshome $env(nshome)
} elseif [file executable ../../../ns] {
	set nshome ../../..
} elseif {[file executable ./ns] || [file executable ./ns.exe]} {
	set nshome "[pwd]"
} else {
	puts "$argv0 cannot find ns directory"
	exit 1
}
set env(PATH) "$nshome/bin:$env(PATH)"

# Clean up procedures
proc finish {} {
  global opt stats on_off_server web_logging_client num_users rate_generator trace_file ns
  for {set i 0} {$i < $num_users} {incr i} {
    # Get link capacity over entire trace
    set user_capacity [expr [$rate_generator get_capacity $i] / 1000000]
    puts [format "User %d, capacity %.3f mbps" $i $user_capacity]
    set sapp $on_off_server($i)
    $sapp dumpstats
    set rcdbytes [$web_logging_client($i) set nbytes_]
    set rcd_nrtt [$web_logging_client($i) set nrtt_]
    if { $rcd_nrtt > 0 } {
        set rcd_avgrtt [expr 1000.0*[$web_logging_client($i) set cumrtt_] / $rcd_nrtt ]
    } else {
        set rcd_avgrtt 0.0
    }
    [$sapp set stats_] showstats $rcdbytes $rcd_avgrtt $user_capacity 
  }
  if { $opt(tracing) == "true" } {
    $ns flush-trace
    close $trace_file
  }
  exit 0
}

# Print out Usage
proc Usage {} {
  global opt argv0
  foreach name [ array names opt ] {
    puts -nonewline stderr " \[-"
    puts -nonewline stderr [ format "%20s" $name ]
    puts -nonewline stderr " :\t"
    puts -nonewline stderr [ format "%15.15s"  $opt($name) ]
    puts -nonewline stderr "\]\n"
  }
}

# Override defaults using the command line if desired
proc Getopt {} {
  global opt argc argv argv0
  if {$argc == 0} {
    puts stderr "Usage : $argv0 \n"
    Usage
    exit 1
  }
  for {set i 0} {$i < $argc} {incr i} {
    set key [lindex $argv $i]
    if ![string match {-*} $key] continue
    set key [string range $key 1 end]
    set val [lindex $argv [incr i]]
    puts stderr "Parameter $key"
    assert ( [ info exists opt($key)] );
    puts stderr "Value $val"
    set opt($key) $val
  }
}

# Function to setup tcp constants 
proc setup_tcp_constants {} {
  global opt
  set delack      0.4
  Agent/TCP set window_     $opt(rcvwin)
  Agent/TCP set segsize_    [expr $opt(pktsize) ]
  Agent/TCP set packetSize_ [expr $opt(pktsize) ]
  Agent/TCP set windowInit_ 4
  Agent/TCP set segsperack_ 1
  Agent/TCP set timestamps_ true
  Agent/TCP set interval_ $delack
  Agent/TCP set tcpTick_ 0.001
  Agent/TCP/FullTcp set window_     $opt(rcvwin)
  Agent/TCP/FullTcp set segsize_    [expr $opt(pktsize)]
  Agent/TCP/FullTcp set packetSize_ [expr $opt(pktsize)]
  Agent/TCP/FullTcp set windowInit_ 4
  Agent/TCP/FullTcp set segsperack_ 1
  Agent/TCP/FullTcp set timestamps_ true
  Agent/TCP/FullTcp set interval_   $delack
  Agent/TCP/FullTcp set tcpTick_ 0.001
  puts "TCP advertised window is [Agent/TCP set window_]"
  puts "FullTcp advertised window is [Agent/TCP/FullTcp set window_]"
}

# Neuter queue
proc neuter_queue {queue} {
  global opt
  # block queues
  $queue set blocked_ 1
  $queue set unblock_on_resume_ 0

  $queue set limit_ $opt(maxq)

  # Deactivate forward queue
  $queue deactivate_queue
}

# Add queue and link to ensemble scheduler
proc attach_to_scheduler {scheduler user queue link} {
  puts stderr "Adding user $user to scheduler"
  $scheduler attach-queue $queue $user
  $scheduler attach-link  $link  $user
  # Deactivate link
  $link deactivate_link
}

# Setup SFD
proc setup_sfd {queue scheduler} {
  $queue attach-sched $scheduler
}

# Link creation
proc create_link {ns latency sender receiver qdisc user_id rate_generator ensemble_scheduler} {
  set bw [$rate_generator get_initial_rate $user_id]
  puts "Initial bandwidth for user $user_id is $bw"
  global opt
  puts "dth $opt(dth)"
  if { $qdisc == "SFD" } {
    set q_args [list $opt(onramp_K) $opt(headroom) $opt(iter) $user_id $opt(droptype) $opt(dth) $opt(percentile) $ensemble_scheduler ]
    $ns simplex-link $sender $receiver [ bw_parse $bw ]  $latency $qdisc $q_args
  } else {
    set q_args [list $ensemble_scheduler ]
    $ns simplex-link $sender $receiver [ bw_parse $bw ]  $latency $qdisc $q_args
  }
  $ns simplex-link $receiver $sender [ bw_parse $opt(ack_bw) ]  $latency DropTail
}

# DRIVER PROGRAM STARTS HERE

# Create a simulator object
set ns [ new Simulator ]

# Clear out opt, in case it already exists
unset opt

# read default constants from config file
source onrampconf/configuration.tcl

# Get options from cmd line
Getopt

# Use Usage function to print out final values of all parameters
Usage

# Now that we have all the parameters set up TCP constants
setup_tcp_constants

# Do we create trace files?
if { $opt(tracing) == "true" } {
  set trace_file [ open $opt(tr) w ] 
  $ns trace-all $trace_file
}

# create basestation
set basestation [ $ns node ]

# Determine number of users
set num_users $opt(nsrc)

# Pick appropriate ensemble_scheduler
if { $opt(sched) == "pf" } {
  set ensemble_scheduler [ new PFScheduler $opt(onramp_K) $num_users 0.0 $opt(cdma_slot) $opt(cdma_ewma) $opt(alpha) $opt(sub_qdisc) ]
} elseif { $opt(sched) == "fcfs" } {
  set ensemble_scheduler [ new FcfsScheduler $opt(onramp_K) $num_users 0.0 ]
}

# Create rate generator
set rate_generator [ new EnsembleRateGenerator $opt(link) $opt(simtime) ]; 
set users_in_trace [ $rate_generator get_users_ ]
puts stderr "Num users is $num_users, users_in_trace $users_in_trace users "
assert [expr $num_users == $users_in_trace]

# Unique ID
set counter 0

# DropTail feedback queue, make sure you have sufficient buffering
Queue set limit_ $opt(maxq)

# TCP connections
source setup_onramp_tcp_connections.tcl

# Activate scheduler
$ns at 0.0 "$ensemble_scheduler activate-link-scheduler"

# Activate rate_generator
$rate_generator activate-rate-generator

# Run simulation
$ns at $opt(simtime) "finish"
$ns run
