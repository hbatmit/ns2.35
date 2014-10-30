# defaults
set ns_tcp(maxburst) 0
set ns_tcp(maxcwnd) 0
set ns_tcp(window) 20
set ns_tcp(window-init) 1
set ns_tcp(window-option) 1
set ns_tcp(window-constant) 4
set ns_tcp(window-thresh) 0.002
set ns_tcp(overhead) 0
set ns_tcp(ecn) 0
set ns_tcp(packet-size) 1000
set ns_tcp(bug-fix) true
set ns_tcp(tcp-tick) 0.1
# These parameters have all been verified: window, window-init,
#   packet-size, bug-fix (tcp, tcp-reno), overhead (tcp but not tcp-reno), 
#   ecn (tcp, tcp-reno, tcp-sink, and tcp-sink-da).  
#   The "overhead" option has not been implemented for tcp-reno.
# The following parameters are for obscure options probably only of interest 
#  to Sally, and have not been tested in the new simulator: window-option, 
#  window-constant, window-thresh.

set ns_red(bytes) false
set ns_red(thresh) 5
set ns_red(maxthresh) 15
set ns_red(mean_pktsize) 500
set ns_red(q_weight) 0.002
set ns_red(wait) true
set ns_red(linterm) 50
set ns_red(setbit) false
set ns_red(drop-tail) false
set ns_red(doubleq) false
set ns_red(dqthresh) 50
set ns_red(subclasses) 1
# These parameters have all been verified: thresh, maxthresh, q_weight,
#  linterm, setbit.
# These parameters are for obscure options probably only of interest to
#  Sally, and have not been tested in the new simulator: doubleq, dqthresh.
# These parameters have not been tested: bytes, mean_pktsize, wait, drop-tail.

# These parameters below are only used for RED queues with RED subclasses.
set ns_red(thresh1) 5
set ns_red(maxthresh1) 15
set ns_red(mean_pktsize1) 500

set ns_cbq(algorithm) 0
set ns_cbq(max-pktsize) 1024
set ns_class(priority) 0
set ns_class(depth) 0
set ns_class(allotment) 0.0
set ns_class(maxidle) 4ms
set ns_class(minidle) -0.2ms
set ns_class(extradelay) 0
set ns_class(plot) false
# These parameters have all been verified: priority, depth, allotment,
#  maxidle, minidle, extradelay.

set ns_sink(packet-size) 40
set ns_delsink(interval) 100ms
set ns_sacksink(max-sack-blocks) 3
set ns_cbr(interval) 3.75ms
set ns_cbr(random) 0
set ns_cbr(packet-size) 210
set ns_rlm(interval) 3.75ms
set ns_rlm(packet-size) 210
set ns_source(maxpkts) 0
set ns_telnet(interval) 1000ms
set ns_bursty(interval) 0
set ns_bursty(burst-size) 2
set ns_message(packet-size) 40

#XXX
set ns_delay(bandwidth) 1.5Mb
set ns_delay(delay) 100ms
set ns_queue(limit) 50

set ns_lossy_uniform(loss_prob) 0.00
# These packets have all been verified: link(bandwidth), link(delay),
#  link(queue-limit), telnet(interval), delsink(interval).

proc ns_connect { src sink } {
	$src connect [$sink addr] [$sink port]
	$sink connect [$src addr] [$src port]
}

proc ns_duplex { n1 n2 bw delay type } {
	set link0 [ns link $n1 $n2 $type]
	$link0 set bandwidth $bw
	$link0 set delay $delay
	set link1 [ns link $n2 $n1 $type]
	$link1 set bandwidth $bw
	$link1 set delay $delay
	return "$link0 $link1"
}

#
# Create a source/sink connection pair and return the source agent.
# 
proc ns_create_connection { srcType srcNode sinkType sinkNode class } {
	set src [ns agent $srcType $srcNode]
	set sink [ns agent $sinkType $sinkNode]
	$src set class $class
	$sink set class $class
	ns_connect $src $sink
	return $src
}

#
# Create a Reno TCP connection with the source/sink pair.  
#   Maximum window $window, start time $start, class $class.
#
proc ns_create_reno { tcpSrc tcpDst window start class} {
        set tcp [ns_create_connection tcp-reno $tcpSrc tcp-sink $tcpDst $class]
        $tcp set window $window
        set ftp [$tcp source ftp]
        ns at $start "$ftp start"
	return $tcp
}

#
# Create a source/sink connection pair for a CBR connection, and return
#   the source agent.
#
proc ns_create_cbr { srcNode sinkNode pktSize interval class } {
	set src [ns agent cbr $srcNode]
	set sink [ns agent loss-monitor $sinkNode]
        $src set interval $interval
        $src set packet-size $pktSize
        $src set class $class
        ns_connect $src $sink
	return $src
}

#
# Procedure for backward compatibility with old ns commands.
# Preferred approach is to allocate objects with the "new" command.
#
proc ns { cmd args } {
	global ns_compat
	if { $cmd == "node" } {
		if ![info exists ns_compat(route-logic)] {
			set ns_compat(route-logic) [new route-logic]
		}
		set node [new node]
		$ns_compat(route-logic) insert $node
		return $node
	}
	if { $cmd == "link" } {
		if { [llength $args] == 3 } {
			set src [lindex $args 0]
			set dst [lindex $args 1]
			set type [lindex $args 2]
			set L [new link $type]
			$L install $src $dst
			set ns_compat(link:$src:$dst) $L
			return $L
		}
		if { [llength $args] == 2 } {
			set src [lindex $args 0]
			set dst [lindex $args 1]
			return $ns_compat(link:$src:$dst)
		}
		if { $args == "" } {
			set L ""
			foreach v [array names ns_compat] {
				if [string match link:* $v] {
					lappend L $ns_compat($v)
				}
			}
			return $L
		}
	}
	if { $cmd == "agent" } {
		if { [llength $args] == 2 } {
			set type [lindex $args 0]
			set node [lindex $args 1]
			set agent [new agent $type]
			$agent node $node
			return $agent
		}
	}
	if { $cmd == "trace" } {
		set trace [new trace]
		return $trace
	}
	if { $cmd == "at" } {
		eval ns-at $args
		return
	}
	if { $cmd == "now" } {
		return [ns-now]
	}
	if { $cmd == "random" } {
		return [eval ns-random $args]
	}
	if { $cmd == "run" } {
		$ns_compat(route-logic) compute-routes
		ns-run
		return
	}
	puts stderr "ns: backward compat doesn't handle 'ns $cmd $args'"
	exit 1
}

#
# XXX hack: the Tcl class calls "tkerror" when it hits an
# error evaluating a tcl stub (e.g., from an ns-at command).
# Dump a stack trace and exit.
# 
proc tkerror s {
	global errorInfo
	puts stderr "ns: tcl eval error"
	puts stderr $errorInfo
	exit 1
}

##########################################################################
# CBQ									 #
##########################################################################

#
# Create a CBQ (class-based queueing) class with the specified parameters.
#
proc ns_create_class { parent borrow allot maxIdle minIdle priority depth extraDelay} { 
        set class [new class]
        set class1 [ns_class_params $class $parent $borrow $allot $maxIdle \
	  $minIdle $priority $depth $extraDelay 0]
        return $class1 
}

# "Mbps" gives the link bandwidth in Mbps.
proc ns_create_class1 { parent borrow allot maxIdle minIdle priority depth extraDelay Mbps} {
        set class [new class]
        set class1 [ns_class_params $class $parent $borrow $allot $maxIdle \
           $minIdle $priority $depth $extraDelay $Mbps]
        return $class1
}  

#
# Assign the following parameters to the CBQ class.
#
proc ns_class_params { class parent borrow allot maxIdle minIdle priority depth extraDelay Mbps} {  
        $class parent $parent
        $class borrow $borrow
        $class set allotment $allot
        $class set maxidle $maxIdle
        $class set minidle $minIdle
        $class set priority $priority
        $class set depth $depth
        $class set extradelay $extraDelay
        set class1 [ns_class_maxIdle $class $allot $maxIdle $priority $Mbps]
        set class2 [ns_class_minIdle $class $allot $minIdle $Mbps]
        return $class2  
}

#
# If $maxIdle is "auto", set maxIdle to Max[t(1/p-1)(1-g^n)/g^n, t(1-g)].
# For p = allotment, t = packet transmission time, g = weight for EWMA.
#
proc ns_class_maxIdle { class allot maxIdle priority Mbps } {
        if { $maxIdle == "auto" } {
		set g 0.9375
		set n [expr 8 * $priority]
		set gTOn [expr pow($g, $n)]
		set first [expr ((1/$allot) - 1) * (1-$gTOn) / $gTOn ]
		set second [expr (1 - $g)]
		set t [expr (1000 * 8)/($Mbps * 1000000) ]
		if { $first > $second } {
		  	$class set maxidle [expr $t * $first]
		} else {
			$class set maxidle [expr $t * $second]
		}
	} else {
		$class set maxidle $maxIdle
	}
	return $class
}

#
# If $minIdle is "auto", set minIdle to t(1/p-1).
# For p = allotment, t = packet transmission time.
#
proc ns_class_minIdle { class allot minIdle Mbps } {
	if { $minIdle == "auto" } {
		set t [expr -(8000 * 8)/($Mbps * 1000000) ]
		$class set minidle [expr $t / $allot]
	} else {
		$class set minidle $minIdle
	}
	return $class
}
