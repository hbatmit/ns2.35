#
# Copyright (c) 2000-2002, by the Rector and Board of Visitors of the 
# University of Virginia.
# All rights reserved.
#
# Redistribution and use in source and binary forms, 
# with or without modification, are permitted provided 
# that the following conditions are met:
#
# Redistributions of source code must retain the above 
# copyright notice, this list of conditions and the following 
# disclaimer. 
#
# Redistributions in binary form must reproduce the above 
# copyright notice, this list of conditions and the following 
# disclaimer in the documentation and/or other materials provided 
# with the distribution. 
#
# Neither the name of the University of Virginia nor the names 
# of its contributors may be used to endorse or promote products 
# derived from this software without specific prior written 
# permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
# DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
# THE POSSIBILITY OF SUCH DAMAGE.
#
# $Id: test-suite-jobs.tcl,v 1.4 2005/06/11 04:42:09 sfloyd Exp $                  
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:

#                                                                     
# This test suite is for validating wireless lans 
# To run all tests: test-all-jobs
# to run individual test:
# ns test-suite-jobs.tcl jobs-rate
# ns test-suite-jobs.tcl jobs-lossdel
#
# To view a list of available test to run with this script:
# ns test-suite-jobs.tcl
#

Class TestSuite

# Rate allocation
Class Test/jobs-rate -superclass TestSuite

# Relative guarantees, TCP traffic
Class Test/jobs-lossdel -superclass TestSuite

# =====================================================================
# General setup
#

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> "
	puts "Valid Tests: jobs-rate jobs-lossdel"
	exit 1
}

proc default_options {} {
	global opt
	set opt(queue_edg1)	Marker
	set opt(queue_edg2)	Demarker
	set opt(queue_core)	JoBS
	set opt(seed)		16.0
	set opt(tr)		temp.rands
	set opt(bw)		10000.0
	set opt(gw_buff)	800
	set opt(pktsz)		500
	set opt(start_tm)	0.0
	set opt(finish_tm)	10.0	
	set opt(hops)		2
	set opt(nsources)	4
	set opt(n_cl1)		1
	set opt(n_cl2)		1
	set opt(n_cl3)		1
	set opt(n_cl4)		1
	set opt(delay)		1.0
	set opt(sample)		1
}

# =====================================================================
# Other default settings
#

Queue/JoBS set drop_front_ false
Queue/JoBS set trace_hop_ false
Queue/JoBS set adc_resolution_type_ 0
Queue/JoBS set shared_buffer_ 0
Queue/JoBS set mean_pkt_size_ 4000
Queue/Demarker set demarker_arrvs1_ 0
Queue/Demarker set demarker_arrvs2_ 0
Queue/Demarker set demarker_arrvs3_ 0
Queue/Demarker set demarker_arrvs4_ 0
Queue/Marker set marker_arrvs1_ 0
Queue/Marker set marker_arrvs2_ 0
Queue/Marker set marker_arrvs3_ 0
Queue/Marker set marker_arrvs4_ 0

# =====================================================================
# The two tests
#

Test/jobs-rate instproc init {} {
    global opt core_node user_source user_source_agent user_sink user_sink_agent 
    $self instvar ns_ testName_
    set testName_       jobs-rate
    set opt(rp)         jobs-rate
    set opt(udp_source_rate) 5000.0

    $self next
    $self jobs-rate-topo
    $self traffic-setup 
    $ns_ at 0.0 "$self record_thru"
    $ns_ at [expr $opt(finish_tm)+.000000001] "puts \"Finishing Simulation...\" ;" 
    $ns_ at [expr $opt(finish_tm)+1] "$self finish"
}

Test/jobs-rate instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}

Test/jobs-lossdel instproc init {} {
    global opt core_node user_source user_source_agent user_sink user_sink_agent 
    $self instvar ns_ testName_
    set testName_       jobs-lossdel
    set opt(rp)         jobs-lossdel
    set opt(udp_source_rate) 2600.0

    $self next
    $self jobs-lossdel-topo
    $self traffic-setup 
    $ns_ at [expr $opt(finish_tm)+.000000001] "puts \"Finishing Simulation...\" ;" 
    $ns_ at [expr $opt(finish_tm)+1] "$self finish"
}

Test/jobs-lossdel instproc run {} {
    $self instvar ns_
    puts "Starting Simulation..."
    $ns_ run
}
#

# =====================================================================
# Traffic generation
#

TestSuite instproc traffic-setup {} {
    global opt user_source user_source_agent user_sink user_sink_agent 
    $self instvar ns_

    for {set i 1} {$i <= $opt(nsources)} {incr i} {
	set user_source_agent($i) [new Agent/UDP]
	set user_sink_agent($i) [new Agent/LossMonitor]
	set user_flow($i) [new Application/Traffic/CBR]
	$ns_ attach-agent $user_source($i) $user_source_agent($i)
	$ns_ attach-agent $user_sink($i) $user_sink_agent($i)
	$ns_ connect $user_source_agent($i) $user_sink_agent($i)
	$user_source_agent($i) set packetSize_ $opt(pktsz)
	$user_flow($i) set rate_ [expr $opt(udp_source_rate)*1000.0]
	$user_flow($i) attach-agent $user_source_agent($i)
	$ns_ at [expr $opt(start_tm)] "$user_flow($i) start"
    }
}

# =====================================================================
# Topology generation
#

TestSuite instproc jobs-rate-topo {} {    
    global opt core_node user_source user_sink
    $self instvar ns_ 
    set DETERM		1

    for {set i 1} {$i <= $opt(hops)} {incr i} {
	set core_node($i) [$ns_ node]
    }


    for {set i 1} {$i <= $opt(nsources)} {incr i} {
	set user_source($i) [$ns_ node]
	set user_sink($i) [$ns_ node]
    }

    for {set i 1} {$i < $opt(hops)} {incr i} {
	$ns_ duplex-link $core_node($i) $core_node([expr $i+1]) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] $opt(queue_core)
	$ns_ queue-limit $core_node($i) $core_node([expr $i+1]) $opt(gw_buff)
	$ns_ queue-limit $core_node([expr $i+1]) $core_node($i) $opt(gw_buff)
	set l [$ns_ get-link $core_node($i) $core_node([expr $i+1])]
	set q [$l queue]

	$q init-rdcs -1 -1 -1 -1
	$q init-rlcs -1 -1 -1 -1 
	$q init-alcs -1 -1 -1 -1 
	$q init-adcs -1 -1 -1 -1 
	$q init-arcs 4000000 3000000 2000000 1000000
	$q link [$l link]
	$q trace-file null
	$q sampling-period $opt(sample)
	$q id $i
	$q initialize
	
	# It's a duplex link, so we need to do exactly the same thing on the 
	# reverse path.
	
	set l [$ns_ get-link $core_node([expr $i+1]) $core_node($i)]
	set q [$l queue]
	
	$q init-rdcs -1 -1 -1 -1
	$q init-rlcs -1 -1 -1 -1 
	$q init-alcs -1 -1 -1 -1 
	$q init-adcs -1 -1 -1 -1 
	$q init-arcs 4000000 3000000 2000000 1000000
	$q link [$l link]
	$q trace-file null
	$q sampling-period $opt(sample)
	$q id [expr $i+$opt(hops)]
	$q initialize
    }

    for {set i 1} {$i <= $opt(nsources)} {incr i} {
	$ns_ simplex-link $user_source($i) $core_node(1) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Marker
	$ns_ simplex-link $core_node(1) $user_source($i) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Demarker
	$ns_ queue-limit $user_source($i) $core_node(1) $opt(gw_buff)
	$ns_ queue-limit $core_node(1) $user_source($i) $opt(gw_buff)

	set q [$ns_ get-queue $user_source($i) $core_node(1)]
	$q marker_type $DETERM

	if {$i == $opt(n_cl1)} {
	    $q marker_class 1
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)} {
	    $q marker_class 2
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)+$opt(n_cl3)} {
	    $q marker_class 3
	} else {
	    $q marker_class 4
	}

	set q [$ns_ get-queue $core_node(1) $user_source($i)]
	$q trace-file null
	
	$ns_ simplex-link $core_node($opt(hops)) $user_sink($i) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Demarker
	$ns_ simplex-link $user_sink($i) $core_node($opt(hops)) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Marker
	$ns_ queue-limit $core_node($opt(hops)) $user_sink($i) $opt(gw_buff)
	$ns_ queue-limit $user_sink($i) $core_node($opt(hops)) $opt(gw_buff)
	
	set q [$ns_ get-queue $core_node($opt(hops)) $user_sink($i)]
	$q trace-file null
	
	set q [$ns_ get-queue $user_sink($i) $core_node($opt(hops))]
	$q marker_type $DETERM
	
	if {$i == $opt(n_cl1)} {
	    $q marker_class 1
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)} {
	    $q marker_class 2
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)+$opt(n_cl3)} {
	    $q marker_class 3
	} else {
	    $q marker_class 4
	}
    }
}

TestSuite instproc jobs-lossdel-topo {} {    
    global opt core_node user_source user_sink
    $self instvar ns_ 
    set DETERM		1

    Queue/JoBS set shared_buffer_ 1
    Queue/JoBS set trace_hop_ true

    for {set i 1} {$i <= $opt(hops)} {incr i} {
	set core_node($i) [$ns_ node]
    }


    for {set i 1} {$i <= $opt(nsources)} {incr i} {
	set user_source($i) [$ns_ node]
	set user_sink($i) [$ns_ node]
    }

    for {set i 1} {$i < $opt(hops)} {incr i} {
	$ns_ duplex-link $core_node($i) $core_node([expr $i+1]) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] $opt(queue_core)
	$ns_ queue-limit $core_node($i) $core_node([expr $i+1]) $opt(gw_buff)
	$ns_ queue-limit $core_node([expr $i+1]) $core_node($i) $opt(gw_buff)
	set l [$ns_ get-link $core_node($i) $core_node([expr $i+1])]
	set q [$l queue]

	$q init-rdcs -1 4 4 4 
	$q init-rlcs -1 2 2 2 
	$q init-alcs 0.01 -1 -1 -1
	$q init-adcs 0.005 -1 -1 -1
	$q init-arcs -1 -1 -1 -1
	$q link [$l link]
	$q trace-file $opt(tr) 
	$q sampling-period $opt(sample)
	$q id $i
	$q initialize
	
	# It's a duplex link, so we need to do exactly the same thing on the 
	# reverse path.
	
	set l [$ns_ get-link $core_node([expr $i+1]) $core_node($i)]
	set q [$l queue]
	
	$q init-rdcs -1 4 4 4 
	$q init-rlcs -1 2 2 2 
	$q init-alcs 0.01 -1 -1 -1
	$q init-adcs 0.005 -1 -1 -1
	$q init-arcs -1 -1 -1 -1
	$q link [$l link]
	$q trace-file null
	$q sampling-period $opt(sample)
	$q id [expr $i+$opt(hops)]
	$q initialize
    }

    for {set i 1} {$i <= $opt(nsources)} {incr i} {
	$ns_ simplex-link $user_source($i) $core_node(1) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Marker
	$ns_ simplex-link $core_node(1) $user_source($i) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Demarker
	$ns_ queue-limit $user_source($i) $core_node(1) $opt(gw_buff)
	$ns_ queue-limit $core_node(1) $user_source($i) $opt(gw_buff)

	set q [$ns_ get-queue $user_source($i) $core_node(1)]
	$q marker_type $DETERM

	if {$i == $opt(n_cl1)} {
	    $q marker_class 1
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)} {
	    $q marker_class 2
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)+$opt(n_cl3)} {
	    $q marker_class 3
	} else {
	    $q marker_class 4
	}

	set q [$ns_ get-queue $core_node(1) $user_source($i)]
	$q trace-file null
	
	$ns_ simplex-link $core_node($opt(hops)) $user_sink($i) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Demarker
	$ns_ simplex-link $user_sink($i) $core_node($opt(hops)) [expr $opt(bw)*1000.] [expr $opt(delay)/1000.] Marker
	 $ns_ queue-limit $core_node($opt(hops)) $user_sink($i) $opt(gw_buff)
	$ns_ queue-limit $user_sink($i) $core_node($opt(hops)) $opt(gw_buff)
	
	set q [$ns_ get-queue $core_node($opt(hops)) $user_sink($i)]
	$q trace-file null
	
	set q [$ns_ get-queue $user_sink($i) $core_node($opt(hops))]
	$q marker_type $DETERM
	
	if {$i == $opt(n_cl1)} {
	    $q marker_class 1
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)} {
	    $q marker_class 2
	} elseif {$i <= $opt(n_cl1)+$opt(n_cl2)+$opt(n_cl3)} {
	    $q marker_class 3
	} else {
	    $q marker_class 4
	}
    }
}


# =====================================================================
# Various helpers
#

Simulator instproc get-link { node1 node2 } {
    $self instvar link_
    set id1 [$node1 id]
    set id2 [$node2 id]
    return $link_($id1:$id2)
}

Simulator instproc get-queue { node1 node2 } {
    set l [$self get-link $node1 $node2]
    set q [$l queue]
    return $q
}

TestSuite instproc init {} {
	global opt tracefd 
	global node_ 
	$self instvar ns_ testName_
	set ns_         [new Simulator]
	set tracefd	[open $opt(tr) w]
}

TestSuite instproc finish {} {
	$self instvar ns_
	global quiet

	$ns_ flush-trace
	puts "finishing.."
	exit 0
}

TestSuite instproc record_thru {} {
    $self instvar ns_
	global tracefd opt user_sink_agent  
	set time 0.5
	set tot_recv 0
	set now [$ns_ now]
	for {set i 1} {$i <= $opt(nsources)} {incr i} {
		set recv($i) [$user_sink_agent($i) set bytes_]
		$user_sink_agent($i) set bytes_ 0
	}
	puts $tracefd [format "%.2f %.2f %.2f %.2f %.2f" $now [expr 8.*$recv(1)/$time] [expr 8.*$recv(2)/$time] [expr 8.*$recv(3)/$time] [expr 8.*$recv(4)/$time]]
	$ns_ at [expr $now+$time] "$self record_thru"
}

# =====================================================================
# Test launcher
#

proc runtest {arg} {
	global quiet
	set quiet 0

	set b [llength $arg]
	if {$b == 1} {
		set test $arg
	} elseif {$b == 2} {
		set test [lindex $arg 0]
		if {[lindex $arg 1] == "QUIET"} {
			set quiet 1
		}
	} else {
		usage
	}
	set t [new Test/$test]
	$t run
}

# =====================================================================
# Main
#

global argv arg0
default_options
runtest $argv









