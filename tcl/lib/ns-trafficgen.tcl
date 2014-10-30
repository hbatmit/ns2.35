#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#Maintainer: Padma Haldar (haldar@isi.edu)

# This class shall hold different traffic models



Class TrafficGen/ManyTCP

TrafficGen/ManyTCP instproc set_raw_opt {} {
    $self instvar raw_opt_info_
    set raw_opt_info_ {
	# how long to run the sim?
	duration 30
	
	# initilization: just start n clients at time 0
	# NEEDSWORK:  a more realistic ramp-up model
	# or some kind of autodetection on when we've reached
	# steady state would be nice.
	initial-client-count 10

	#
	# BASIC TOPOLOGY:
	# TOPO1 :
	# (The basic n clients on the left and right going through a
	# bottleneck.)
	#
	# cl_1                                                   cr_1
	# ...     ---- bottleneck_left ---- bottleneck_right --- ...
	# cl_n                                                   cr_n
	#
	# node-number 0 specifies a new pair of nodes
	# on the left and right for each new client
	node-number 0
	# NEEDSWORK:
	# The number of agents attached to a node cannot exceed
	# the port-field length (255 bits).  There is currently
	# no check or warning message for this.
	#
	# Currently all data traffic flows left-to-right.
	# NEEDSWORK: relax this assumption (but Poduri and Nichols
	# I-D suggests that relaxing it won't change things much).
	#
	client-nodes-l -1
	client-nodes-r -1
	#NEEDSWORK :
	# TOPO2: connect client nodes from a set A to set B
	# TOPO3: N% nodes from stubs or total nodes in topology.--this
	# has been implemented already in the topology generator and 
	# needs to be hooked here.
	#
	#Bottleneck link
	bottle_link_l -1
	bottle_link_r -1
	#
	#
	# CLIENT TRAFFIC MODEL:
	#
	# arrival rate per second (arrival is poisson)
	client-arrival-rate 1
	# Currently clients are either mice or elephants.
	# NEEDSWORK:  should better model http-like traffic patterns.
	# In particular, netscape's 4-connection model makes
	# a *big* difference in traffic patterns
	# and is not currently modeled at all.
	client-mouse-chance 90
	client-mouse-packets 10
	client-elephant-packets 100
	# For traffic in the reverse direction.
	client-reverse-chance 0
	# Pkt size in bytes.
	# NEEDSWORK:  should check that everything is uniformly
	# specified (router queues are in packets of 1000B length?).
	client-pkt-size 576

	#
	# CLIENT NETWORK CONNECTION:
	#
	client-bw 56kb
	# client-server rtt is uniform over this range (currently)
	# NEEDSWORK:  does this need to be modeled more accurately?
	client-delay random
	client-delay-range 100ms
	client-queue-method DropTail
	# Insure that client routers are never a bottleneck.
	client-queue-length 100

	#
	# CLIENT/SERVER TCP IMPLEMENTATION:
	#
	# NEEDSWORK: should add HTTP model over TCP.
	source-tcp-method TCP/Reno
	sink-ack-method TCPSink/DelAck
	# Set init-win to 1 for initial windows of size 1.
	# Set init-win to 10 for initial windows of 10 packets.
	# Set init-win to 0 for initial windows per internet-draft.
	init-win 1

	#
	# BOTTLENECK LINK MODEL:
	#
	bottle-bw 10Mb
	bottle-delay 4ms
	bottle-queue-method RED
	# bottle-queue-length is either in packets or
	# is "bw-delay-product" which does the currently
	# expected thing.
	bottle-queue-length bw-delay-product

	#
	# OUTPUT OPTIONS:
	#
	graph-results 0
	# Set graph-scale to 2 for "rows" for each flow.
	graph-scale 1
	graph-join-queueing 1
	gen-map 0
	mem-trace 0
	print-drop-rate 0
	title none
	# set test-suite to write the graph to opts_(test-suite-file)
	test-suite 0
	test-suite-file temp.rands
		   
	# Random number seed; default is 0, so ns will give a 
	# diff. one on each invocation.
	ns-random-seed 0
	
	# Animation options; complete traces are useful
	# for nam only, so do those only when a tracefile
	# is being used for nam
	# Set trace-filename to "none" for no tracefile.
	trace-filename out
	trace-all 0
	namtrace-some 0
	namtrace-all 0
	
	# Switch to generate the nam tcl file from here
	# itself
	nam-generate-cmdfile 0
    }
}

TrafficGen/ManyTCP instproc default_options {} {
    $self instvar opts_ opt_wants_arg_ raw_opt_info_
    set cooked_opt_info $raw_opt_info_

    while {$cooked_opt_info != ""} {
	if {![regexp "^\[^\n\]*\n" $cooked_opt_info line]} {
	    break
	}
	regsub "^\[^\n\]*\n" $cooked_opt_info {} cooked_opt_info
	set line [string trim $line]
	if {[regexp "^\[ \t\]*#" $line]} {
	    continue
	}
	if {$line == ""} {
	    continue
	} elseif [regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value] {
	    set opts_($key) $value
	    set opt_wants_arg_($key) 1
	} else {
	    set opt_wants_arg_($key) 0
	    # die "unknown stuff in raw_opt_info_\n"
	}
    }
}

TrafficGen/ManyTCP instproc process_args {av} {
    $self instvar opts_ opt_wants_arg_

    $self default_options
    for {set i 0} {$i < [llength $av]} {incr i} {
	set key [lindex $av $i]
	if {$key == "-?" || $key == "--help" || $key == "-help" || $key == "-h"} {
	    usage
	}
	regsub {^-} $key {} key
	if {![info exists opt_wants_arg_($key)]} {
	    puts stderr "unknown option $key";
	    usage
	}
	
	if {$opt_wants_arg_($key)} {
	    incr i
	    set opts_($key) [lindex $av $i]
	} else {
	    set opts_($key) [expr !opts_($key)]
	}
    }
}

TrafficGen/ManyTCP instproc get_list {list name} {
    $self instvar opts_
    for {set i 0} {$i < [llength $list]} {incr i} {
	set $opts_($name:$i) [lindex $list $i]
    }
}


TrafficGen/ManyTCP instproc readin_client_nodes {} {
    $self instvar opts_
    if {$opts_(client-nodes-l) != -1} {
	set temp [split $opts_(client-nodes-l) ,]
	$self get_list $temp client-nodes-l
    }
    if {$opts_(client-nodes-r) != -1} {
	set temp [split $opts_(client-nodes-r) ,]
	$self get_list $temp client-nodes-r
    }
}

proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {
    
    $ns duplex-link $n1 $n2 $bw $delay $queue_method
    $ns queue-limit $n1 $n2 $queue_length
    $ns queue-limit $n2 $n1 $queue_length
}


TrafficGen/ManyTCP instproc init_network {} {
    $self instvar opts_ fmon_ bottle_l_ bottle_r_ cs_l_ cs_r_ ns_ \
	    cs_count_ ns_ clients_started_ clients_finished_
    
    #
    # Figure supported load.
    #
    set expected_load_per_client_in_bps [expr ($opts_(client-mouse-chance)/100.0)*$opts_(client-mouse-packets)*$opts_(client-pkt-size)*8 + (1.0-$opts_(client-mouse-chance)/100.0)*$opts_(client-elephant-packets)*$opts_(client-pkt-size)*8]
    if {$opts_(debug)} {
	set max_clients_per_second [expr [bw_parse $opts_(bottle-bw)]/$expected_load_per_client_in_bps]
	puts [format "maximum clients per second: %.3f" $max_clients_per_second]
    }
    
    # Compute optimal (?) bottleneck queue size
    # as the bw-delay product.
    if {$opts_(bottle-queue-length) == "bw-delay-product"} {
	set opts_(bottle-queue-length) [expr ([bw_parse $opts_(bottle-bw)] * ([time_parse $opts_(bottle-delay)] + [time_parse $opts_(client-delay-range)]) + $opts_(client-pkt-size)*8 - 1)/ ($opts_(client-pkt-size) * 8)]
	puts "optimal bw queue size: $opts_(bottle-queue-length)"
    }
    
    # set up the bottleneck
    if {$opts_(bottle_link_l) != -1 || $opts_(bottle_link_r) != -1} {
	set bottle_l_ $opts_(bottle_link_l)
	set bottle_r_ $opts_(bottle_link_r)
    } else {
	$ns_ rtproto Manual
	set bottle_l_ [$ns_ node]
	set bottle_r_ [$ns_ node]
    }
    my-duplex-link $ns_ $bottle_l_ $bottle_r_ $opts_(bottle-bw) $opts_(bottle-delay) $opts_(bottle-queue-method) $opts_(bottle-queue-length)
    if {$opts_(print-drop-rate)} {
	set slink [$ns_ link $bottle_l_ $bottle_r_]
	set fmon_ [$ns_ makeflowmon Fid]
	$ns_ attach-fmon $slink $fmon_
    }
	
    # Bottlenecks need large routing tables.
    # [$bottle_l_ set classifier_] resize 511
    # [$bottle_r_ set classifier_] resize 511
    
    # Default routes to the other.
    $bottle_l_ add-route-to-adj-node -default $bottle_r_
    $bottle_r_ add-route-to-adj-node -default $bottle_l_
    
    # Clients are built dynamically.
    set cs_count_ 0
    set clients_started_ 0
    set clients_finished_ 0
}

TrafficGen/ManyTCP instproc init {args} {
    $self instvar opts_ ns_ rng_

    set ns_ [Simulator instance]
    $self set_raw_opt
    $self process_args $args
    
    # Seed random no. generator; ns-random with arg of 0 heuristically
    # chooses a random number that changes on each invocation.
    $self instvar rng_
    set rng_ [new RNG]
    $rng_ seed $opts_(ns-random-seed)
    $rng_ next-random

    $self init_network
}

# create a new pair of end nodes
TrafficGen/ManyTCP instproc create_client_nodes {node} {
    $self instvar opts_ bottle_l_ bottle_r_ cs_l_ cs_r_ sources_ \
	    cs_count_ ns_ rng_

    set now [$ns_ now]
    if {![info exists opts_(client-nodes-l:$node)]} {
	set cs_l_($node) [$ns_ node]
    } else {
	set cs_l_($node) $opts(client-nodes-l:$node)
    }
    if {![info exists opts_(client-nodes-r:$node)]} {
	set cs_r_($node) [$ns_ node]
    } else {
	set cs_r_($node) $opts(client-nodes-r:$node)
    }
    # Set delay.
    set delay $opts_(client-delay)
    if {$delay == "random"} {
	set delay [$rng_ exponential [time_parse $opts_(client-delay-range)]]
    }
    # Now divide the delay into the two haves and set up the network.
    set ldelay [$rng_ uniform 0 $delay]
    set rdelay [expr $delay - $ldelay]
    
    my-duplex-link $ns_ $cs_l_($node) $bottle_l_ $opts_(client-bw) $ldelay $opts_(client-queue-method) $opts_(client-queue-length)
    my-duplex-link $ns_ $cs_r_($node) $bottle_r_ $opts_(client-bw) $rdelay $opts_(client-queue-method) $opts_(client-queue-length)
    
    # Add routing in all directions
    $cs_l_($node) add-route-to-adj-node -default $bottle_l_
    $cs_r_($node) add-route-to-adj-node -default $bottle_r_
    $bottle_l_ add-route-to-adj-node $cs_l_($node)
    $bottle_r_ add-route-to-adj-node $cs_r_($node)
    
    if {$opts_(debug)} {
	# puts "t=[format %.3f $now]: node pair $node created"
	# puts "delay $delay ldelay $ldelay"
    }
}

# Get the number of the node pair
TrafficGen/ManyTCP instproc get_node_number { client_number } {
        $self instvar opts_
    if {$opts_(node-number) > 0} {
	set node [expr $client_number % $opts_(node-number)]
    } else {
	set node $client_number
    }
    return $node
}

# return the client index
TrafficGen/ManyTCP instproc create_a_client {} {
    $self instvar opts_ cs_l_ cs_r_ sources_ cs_count_ ns_ rng_

    # Get the client number for the new client.
    set now [$ns_ now]
    set i $cs_count_
    incr cs_count_
    set node $i
    if {[expr $i % 100] == 0} {
	puts "t=[format %.3f $now]: client $i created"
    }

    # Get the source and sink nodes.
    if {$opts_(node-number) > 0} {
	if {$node < $opts_(node-number) } {
	    $self create_client_nodes $node
	} else {
	    set node [$self get_node_number $i]
	}
    } else {
	$self create_client_nodes $node
    }
    if {$opts_(debug)} {
	# puts "t=[format %.3f $now]: client $i uses node pair $node"
    }
    
    # create sources and sinks in both directions
    # (actually, only one source per connection, for now)
    if {[$rng_ integer 100] < $opts_(client-reverse-chance)} {
	set sources_($i) [$ns_ create-connection-list $opts_(source-tcp-method) $cs_r_($node) $opts_(sink-ack-method) $cs_l_($node) $i]
    } else {
	set sources_($i) [$ns_ create-connection-list $opts_(source-tcp-method) $cs_l_($node) $opts_(sink-ack-method) $cs_r_($node) $i]
    }
    [lindex $sources_($i) 0] set maxpkts_ 0
    [lindex $sources_($i) 0] set packetSize_ $opts_(client-pkt-size)
    
    # Set up a callback when this client ends.
    [lindex $sources_($i) 0] proc done {} "$self finish_a_client $i"
    
    if {$opts_(debug)} {
	# puts "t=[$ns_ now]: client $i created"
    }
    
	return $i
}

#
# Make a batch of clients to amortize the cost of routing recomputation
# (actually no longer improtant).
#
TrafficGen/ManyTCP instproc create_some_clients {} {
    $self instvar opts_ idle_clients_ ns_ cs_count_

    set now [$ns_ now]
    set step 16
    if {$opts_(debug)} {
	puts "t=[format %.3f $now]: creating clients $cs_count_ to [expr $cs_count_ + $step - 1]"
    }
    
    for {set i 0} {$i < $step} {incr i} {
	lappend idle_clients_ [$self create_a_client]
    }
    
    # debugging:
    # puts "after client_create:"
    # $ns_ gen-map
    # $self instvar bottle_l_ bottle_r_
    # puts "bottle_l_ classifier_:"
    # [$bottle_l_ set classifier_] dump
    # puts "bottle_r_ classifier_:"
    # [$bottle_r_ set classifier_] dump
}

TrafficGen/ManyTCP instproc start_a_client {} {
    $self instvar opts_ idle_clients_ ns_ sources_ rng_ \
	    source_start_ source_size_ clients_started_

	set i ""
	set now [$ns_ now]
	# can we reuse a dead client?
	if {![info exists idle_clients_]} {
		set idle_clients_ ""
	}
	while {$idle_clients_ == ""} {
		$self create_some_clients
	}
	set i [lindex $idle_clients_ 0]
	set idle_clients_ [lrange $idle_clients_ 1 end]

	# Reset the connection.
	[lindex $sources_($i) 0] reset
	[lindex $sources_($i) 1] reset 

	# Start traffic for that client.
	if {[$rng_ integer 100] < $opts_(client-mouse-chance)} {
		set len $opts_(client-mouse-packets)
	} else {
		set len $opts_(client-elephant-packets)
	}

	[lindex $sources_($i) 0] advanceby $len
	set source_start_($i) $now
	set source_size_($i) $len

	if {$opts_(debug)} {
		 # puts "t=[$ns_ now]: client $i started, ldelay=[format %.6f $ldelay], rdelay=[format %.6f $rdelay]"
		puts "t=[format %.3f $now]: client $i started"
	}
	incr clients_started_
}

TrafficGen/ManyTCP instproc finish_a_client {i} {
    $self instvar opts_ ns_ idle_clients_ source_start_ source_size_ \
	    clients_finished_

    set now [$ns_ now]
    if {$opts_(debug)} {
	set delta [expr $now - $source_start_($i)]
	puts "t=[format %.3f $now]: client $i finished ($source_size_($i) pkts, $delta s)"
    }
    
    lappend idle_clients_ $i
    incr clients_finished_
}

TrafficGen/ManyTCP instproc schedule_continuing_traffic {} {
    $self instvar opts_ ns_ rng_

    $self start_a_client
    # schedule the next one
    set next [expr [$ns_ now]+([$rng_ exponential]/$opts_(client-arrival-rate))]
    if {$opts_(debug)} {
	# puts "t=[$ns_ now]: next continuing traffic at $next"
    }
    $ns_ at $next "$self schedule_continuing_traffic"
}

TrafficGen/ManyTCP instproc schedule_initial_traffic {} {
    $self instvar opts_ idle_clients_
    
    # Start with no pending clients.
    
    # Start initial clients.
    for {set i 0} {$i < $opts_(initial-client-count)} {incr i} {
	$self start_a_client
    }
}

TrafficGen/ManyTCP instproc open_trace { stop_time } {
    $self instvar opts_ ns_ trace_file_ nam_trace_file_ \
	    trace_filename_
    set trace_filename_ $opts_(trace-filename)
    exec rm -f "$trace_filename_.tr"
    set trace_file_ [open "$trace_filename_.tr" w]
    set stop_actions "close $trace_file_"
    if {$opts_(namtrace-some) || $opts_(namtrace-all)} {
	exec rm -f "$trace_filename_.nam"
	set nam_trace_file_ [open "$trace_filename_.nam" w]
	set $stop_actions "$stop_actions; close $nam_trace_file_"
    } else {
	set nam_trace_file_ ""
    }
    $ns_ at $stop_time "$stop_actions; $self finish"
    return "$trace_file_ $nam_trace_file_"
}

# There seems to be a problem with the foll function, so quit plotting 
# with -a -q, use just -a.

TrafficGen/ManyTCP instproc finish {} {
    $self instvar opts_ fmon_ trace_filename_ ns_ cs_count_ \
	    clients_started_ clients_finished_


    puts "total clients started: $clients_started_"
    puts "total clients finished: $clients_finished_"
    if {$opts_(print-drop-rate)} {
	set drops [$fmon_ set pdrops_]
	set pkts [$fmon_ set parrivals_]
	puts "total_drops $drops total_packets $pkts"
	set droprate [expr 100.0*$drops / $pkts ]
	puts [format "drop_percentage %7.4f" $droprate]
    }
    if {$opts_(trace-filename) != "none"} {
	set title $opts_(title)
	set flow_factor 1
	if {$opts_(graph-scale) == "2"} {
	    set flow_factor 100
                }
		# Make sure that we run in place even without raw2xg in our path
		# (for the test suites).
		set raw2xg raw2xg
		if [file exists ../../bin/raw2xg] {
			set raw2xg ../../bin/raw2xg
		}
		set raw2xg_opts ""
		if {$opts_(graph-join-queueing)} {
			set raw2xg_opts "$raw2xg_opts -q"
		}
		# always run raw2xg because maybe we need the output
		set cmd "$raw2xg -a $raw2xg_opts -n $flow_factor < $trace_filename_.tr >$trace_filename_.xg"
		eval "exec $cmd"
		if {$opts_(graph-results)} {
			if {$opts_(graph-join-queueing)} {
				exec xgraph -t $title  < $trace_filename_.xg &
			} else {
				exec xgraph -tk -nl -m -bb -t $title < $trace_filename_.xg &
			}
		}
		if {$opts_(test-suite)} {
			exec cp $trace_filename_.xg $opts_(test-suite-file)
		}
	#	exec raw2xg -a < out.tr | xgraph -t "$opts_(server-tcp-method)" &
	}

	if {$opts_(mem-trace)} {
		$ns_ clearMemTrace
	}
	exit 0
}

TrafficGen/ManyTCP instproc trace_stuff {} {
    $self instvar opts_ bottle_l_ bottle_r_ ns_ trace_file_ \
	    nam_trace_file_
	$self open_trace $opts_(duration)

	if {$opts_(trace-all)} {
		$ns_ trace-all $trace_file_
	}

	if {$opts_(namtrace-all)} {
		$ns_ namtrace-all $nam_trace_file_
	} elseif {$opts_(namtrace-some)} {
# xxx
		$bottle_l_ dump-namconfig
		$bottle_r_ dump-namconfig
		[$ns_ link $bottle_l_ $bottle_r_] dump-namconfig
		$ns_ namtrace-queue $bottle_l_ $bottle_r_ $nam_trace_file_
		$ns_ namtrace-queue $bottle_r_ $bottle_l_ $nam_trace_file_
	}

	# regular tracing.
	# trace left-to-right only
       	$ns_ trace-queue $bottle_l_ $bottle_r_ $trace_file_
       	$ns_ trace-queue $bottle_r_ $bottle_l_ $trace_file_
	
	# Currently tracing is somewhat broken because
	# of how the plumbing happens.
}



TrafficGen/ManyTCP instproc start {} {
    $self instvar ns_ opts_

    if {$opts_(trace-filename) == "none"} {
	$ns_ at $opts_(duration) "$self finish"
    } else {
	$self trace_stuff
    }
    if {$opts_(gen-map)} {
	$ns_ gen-map
    }
    
    $self schedule_initial_traffic
    if {$opts_(client-arrival-rate) != 0} {
	$self schedule_continuing_traffic
    }
    
    if {$opts_(gen-map)} {
	$ns_ gen-map
    }       
    Agent/TCP set syn_ true
    Agent/TCP set delay_growth_ true
    Agent/TCP set windowInit_ 1
    Agent/TCP set windowInitOption_ 1
    if {$opts_(init-win) == "0"} {
	Agent/TCP set windowInitOption_ 2
    } elseif {$opts_(init-win) == "10"} {
	Agent/TCP set windowInitOption_ 1
	Agent/TCP set windowInit_ 10
    } elseif {$opts_(init-win) == "20"} {
	Agent/TCP set windowInitOption_ 1
	Agent/TCP set windowInit_ 20
	puts "init-win 20"
    }
}


