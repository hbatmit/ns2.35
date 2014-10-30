# Test suite for HTTP server, client, proxy cache.
#
# Also tests TcpApp, which is an Application used to transmit 
# application-level data. Because current TCP isn't capable of this,
# we build this functionality based on byte-stream model of underlying 
# TCP connection.
# 
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-webcache.tcl,v 1.26 2011/10/02 22:31:14 tom_henderson Exp $

#----------------------------------------------------------------------
# Related Files
#----------------------------------------------------------------------
source misc.tcl
source topologies.tcl

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP HttpInval ; # hdrs reqd for validation test

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set exitFastRetrans_ false
#
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set timerfix_ false
# The default is being changed to true.
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

#----------------------------------------------------------------------
# Misc setup
#----------------------------------------------------------------------
set tcl_precision 10



#----------------------------------------------------------------------
# Topologies for cache testing
#----------------------------------------------------------------------

# Simplest topology: 1 client + 1 cache + 1 server
Class Topology/cache0 -superclass SkelTopology

Topology/cache0 instproc init ns {
	$self next
	$self instvar node_

	set node_(c) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	$ns duplex-link $node_(s) $node_(e) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e) $node_(c) 10Mb 2ms DropTail
	$ns duplex-link-op $node_(c) $node_(e) orient right
	$ns duplex-link-op $node_(e) $node_(s) orient right
}

# Hierarchical cache, 1 server + 7 cache + 4 clients, server linked to 
# a top-level cache
Class Topology/cache2 -superclass SkelTopology

Topology/cache2 instproc init ns {
	$self next
	$self instvar node_

	set node_(c0) [$ns node]
	set node_(c1) [$ns node]
	set node_(c2) [$ns node]
	set node_(c3) [$ns node]
	set node_(e0) [$ns node]
	set node_(e1) [$ns node]
	set node_(e2) [$ns node]
	set node_(e3) [$ns node]
	set node_(e4) [$ns node]
	set node_(e5) [$ns node]
	set node_(e6) [$ns node]
	set node_(s0) [$ns node]

	# between top-level cache: OC3
	$ns duplex-link $node_(e0) $node_(e1) 155Mb 100ms DropTail
	# server to top-level cache and inside a cache hierarchy: T1
	$ns duplex-link $node_(s0) $node_(e0) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e0) $node_(e2) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e0) $node_(e3) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e1) $node_(e4) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e1) $node_(e5) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e2) $node_(e6) 10Mb 2ms DropTail
	# client to caches: 10Mb ethernet
	$ns duplex-link $node_(e2) $node_(c0) 10Mb 2ms DropTail
	$ns duplex-link $node_(e6) $node_(c1) 10Mb 2ms DropTail
	$ns duplex-link $node_(e4) $node_(c2) 10Mb 2ms DropTail
	$ns duplex-link $node_(e1) $node_(c3) 10Mb 2ms DropTail

	$ns duplex-link-op $node_(s0) $node_(e0) orient right
	$ns duplex-link-op $node_(e0) $node_(e1) orient right
	$ns duplex-link-op $node_(e0) $node_(e2) orient left-down
	$ns duplex-link-op $node_(e0) $node_(e3) orient right-down
	$ns duplex-link-op $node_(e2) $node_(e6) orient down
	$ns duplex-link-op $node_(c0) $node_(e2) orient right
	$ns duplex-link-op $node_(c1) $node_(e6) orient right
	$ns duplex-link-op $node_(e1) $node_(e4) orient left-down
	$ns duplex-link-op $node_(e1) $node_(e5) orient right-down
	$ns duplex-link-op $node_(e4) $node_(c2) orient down
	$ns duplex-link-op $node_(e1) $node_(c3) orient right

	$self checkConfig $class $ns
}

# Hierarchical cache, 1 server + 7 cache + 4 clients, server linked to a 
# second-level cache.
Class Topology/cache3 -superclass SkelTopology

Topology/cache3 instproc init ns {
	$self next
	$self instvar node_

	set node_(c0) [$ns node]
	set node_(c1) [$ns node]
	set node_(c2) [$ns node]
	set node_(c3) [$ns node]
	set node_(e0) [$ns node]
	set node_(e1) [$ns node]
	set node_(e2) [$ns node]
	set node_(e3) [$ns node]
	set node_(e4) [$ns node]
	set node_(e5) [$ns node]
	set node_(e6) [$ns node]
	set node_(s0) [$ns node]

	# between top-level cache: OC3
	$ns duplex-link $node_(e0) $node_(e1) 155Mb 100ms DropTail
	# server to top-level cache and inside a cache hierarchy: T1
	$ns duplex-link $node_(s0) $node_(e5) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e0) $node_(e2) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e0) $node_(e3) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e1) $node_(e4) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e1) $node_(e5) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e2) $node_(e6) 10Mb 2ms DropTail
	# client to caches: 10Mb ethernet
	$ns duplex-link $node_(e2) $node_(c0) 10Mb 2ms DropTail
	$ns duplex-link $node_(e6) $node_(c1) 10Mb 2ms DropTail
	$ns duplex-link $node_(e4) $node_(c2) 10Mb 2ms DropTail
	$ns duplex-link $node_(e1) $node_(c3) 10Mb 2ms DropTail

	$ns duplex-link-op $node_(e5) $node_(s0) orient right
	$ns duplex-link-op $node_(e0) $node_(e1) orient right
	$ns duplex-link-op $node_(e0) $node_(e2) orient left-down
	$ns duplex-link-op $node_(e0) $node_(e3) orient right-down
	$ns duplex-link-op $node_(e2) $node_(e6) orient down
	$ns duplex-link-op $node_(c0) $node_(e2) orient right
	$ns duplex-link-op $node_(c1) $node_(e6) orient right
	$ns duplex-link-op $node_(e1) $node_(e4) orient left-down
	$ns duplex-link-op $node_(e1) $node_(e5) orient right-down
	$ns duplex-link-op $node_(e4) $node_(c2) orient down
	$ns duplex-link-op $node_(e1) $node_(c3) orient right

	$self checkConfig $class $ns
}

# Two level hierarchical cache. 1 server + 1 TLC + n 2nd caches with one 
# bottleneck link connecting TCL to other caches + n clients
Class Topology/BottleNeck -superclass SkelTopology

Class Topology/BottleNeck -superclass SkelTopology

Topology/BottleNeck instproc init { ns } {
	$self next 
	$self instvar node_ 

	global opts
	if [info exists opts(num-2nd-cache)] {
		set n $opts(num-2nd-cache)
	} else {
		error "Topology/BottleNeck requires option num-2nd-cache"
	}

	set node_(s0) [$ns node]
	# TLC is node e0
	for {set i 0} {$i <= $n} {incr i} {
		set node_(e$i) [$ns node]
	}
	# We create clients separately so we have consecutive ids for all 
	# clients
	for {set i 0} {$i < $n} {incr i} {
		set node_(c$i) [$ns node]
	}

	# Between TLC and server: T1
#	$ns duplex-link $node_(e$n) $node_(s0) 1.5Mb 100ms DropTail

	# Server attached to a client via a LAN
	$ns duplex-link $node_(e0) $node_(s0) 1.5Mb 100ms DropTail
	#$ns duplex-link $node_(e0) $node_(s0) 10Mb 2ms DropTail

	# Bottleneck link
	$self instvar dummy_
	set dummy_ [$ns node]
	$ns duplex-link $node_(e$n) $dummy_ 1.5Mb 50ms DropTail

	for {set i 0} {$i < $n} {incr i} {
		$ns duplex-link $node_(e$i) $dummy_ 1.5Mb 50ms DropTail
		$ns duplex-link $node_(c$i) $node_(e$i) 10Mb 2ms DropTail
	}

	$self checkConfig $class $ns
}

Topology/BottleNeck instproc start-monitor { ns } {
	$self instvar qmon_ node_ dummy_

	# Traffic between server and its primary cache
	set qmon_(svr_f) [$ns monitor-queue $node_(s0) $node_(e0) ""]
	set qmon_(svr_t) [$ns monitor-queue $node_(e0) $node_(s0) ""]

	global opts
	set n $opts(num-2nd-cache)

	# Traffic between TLC and all others
	set qmon_(btnk_f) [$ns monitor-queue $node_(e$n) $dummy_ ""]
	set qmon_(btnk_t) [$ns monitor-queue $dummy_ $node_(e$n) ""]

	# Traffic for all the rest links
	for {set i 0} {$i < $n} {incr i} {
		set qmon_(e${i}_d_f) [$ns monitor-queue $node_(e$i) $dummy_ ""]
		set qmon_(e${i}_d_t) [$ns monitor-queue $dummy_ $node_(e$i) ""]
		set qmon_(e${i}_c${i}_f) \
			[$ns monitor-queue $node_(e$i) $node_(c$i) ""]
		set qmon_(e${i}_c${i}_t) \
			[$ns monitor-queue $node_(c$i) $node_(e$i) ""]
	}
	#puts "Monitors started at time [$ns now]"
}

Topology/BottleNeck instproc mon-stat {} {
	$self instvar qmon_

	set total_bw 0
	foreach n [array names qmon_] {
		set total_bw [expr $total_bw + \
			double([$qmon_($n) set bdepartures_])]
	}
	set svr_bw [expr double([$qmon_(svr_f) set bdepartures_]) + \
			double([$qmon_(svr_t) set bdepartures_])]
	set btnk_bw [expr double([$qmon_(btnk_f) set bdepartures_]) + \
			double([$qmon_(btnk_t) set bdepartures_])]
	return [list total_bw $total_bw svr_bw $svr_bw btnk_bw $btnk_bw]
}

#
# Three level hierarchical cache, binary tree. 
#
Class Topology/cache4 -superclass SkelTopology

Topology/cache4 instproc init { ns } {
	$self next
	$self instvar node_

	# server attached to a leaf cache
	set node_(s0) [$ns node]
	# TLC is node e0
	for {set i 0} {$i <= 6} {incr i} {
		set node_(e$i) [$ns node]
	}
	# All clients attached to leaf caches
	for {set i 0} {$i <= 3} {incr i} {
		set node_(c$i) [$ns node]
	}

	# Bottleneck link between TLC and other caches
	set dummy [$ns node]
	$ns duplex-link $node_(e0) $dummy 100Mb 1ms DropTail
	$ns duplex-link $dummy $node_(e1) 1.5Mb 50ms DropTail
	$ns duplex-link $dummy $node_(e2) 1.5Mb 50ms DropTail

	$ns duplex-link $node_(e1) $node_(e3) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(e1) $node_(e4) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(e2) $node_(e5) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(e2) $node_(e6) 1.5Mb 10ms DropTail
	$ns duplex-link $node_(e3) $node_(c0) 10Mb 1ms DropTail
	$ns duplex-link $node_(e4) $node_(c1) 10Mb 1ms DropTail
	$ns duplex-link $node_(e5) $node_(c2) 10Mb 1ms DropTail
	$ns duplex-link $node_(e6) $node_(c3) 10Mb 1ms DropTail
	$ns duplex-link $node_(s0) $node_(e3) 10Mb 10ms DropTail
	
	$ns duplex-link-op $node_(e0) $dummy orient down
	$ns duplex-link-op $dummy $node_(e1) orient left-down
	$ns duplex-link-op $dummy $node_(e2) orient right-down
	$ns duplex-link-op $node_(e1) $node_(e3) orient left-down
	$ns duplex-link-op $node_(e1) $node_(e4) orient right-down
	$ns duplex-link-op $node_(e2) $node_(e5) orient left-down
	$ns duplex-link-op $node_(e2) $node_(e6) orient right-down
	$ns duplex-link-op $node_(e3) $node_(c0) orient down
	$ns duplex-link-op $node_(e4) $node_(c1) orient down
	$ns duplex-link-op $node_(e5) $node_(c2) orient down
	$ns duplex-link-op $node_(e6) $node_(c3) orient down
	$ns duplex-link-op $node_(s0) $node_(e3) orient right

	$self checkConfig $class $ns
}

# Same as Topology/cache4, except adding a dynamic links
Class Topology/cache4d -superclass Topology/cache4

Topology/cache4d instproc init { ns } {
	$self next $ns
	$self instvar node_
	$ns rtmodel-at 500 down $node_(s0) $node_(e3)
	$ns rtmodel-at 1000 up $node_(s0) $node_(e3)
	$self checkConfig $class $ns
}

# 2-level topology with direct links from server to every client
# Compare invalidation vs ttl with direct request
Class Topology/cache5 -superclass SkelTopology

Topology/cache5 instproc init { ns } {
	$self next
	$self instvar node_

	global opts
	if [info exists opts(num-2nd-cache)] {
		set n $opts(num-2nd-cache)
	} else {
		error "Topology/BottleNeck requires option num-2nd-cache"
	}

	set node_(s0) [$ns node]
	# TLC is node e0
	for {set i 0} {$i <= $n} {incr i} {
		set node_(e$i) [$ns node]
	}
	# We create clients separately so we have consecutive ids for all 
	# clients
	for {set i 0} {$i < $n} {incr i} {
		set node_(c$i) [$ns node]
	}

	set sn [$ns node]	;# Dummy node for bottleneck link
	$ns duplex-link $node_(e$n) $sn 1.5Mb 50ms DropTail
	# Traffic on the duplex link. 
	$self instvar qmon_
	set qmon_(btnk_f) [$ns monitor-queue $node_(e$n) $sn ""]
	set qmon_(btnk_t) [$ns monitor-queue $sn $node_(e$n) ""]

	for {set i 0} {$i < $n} {incr i} {
		$ns duplex-link $node_(e$i) $sn 1.5Mb 50ms DropTail
		$ns duplex-link $node_(c$i) $node_(e$i) 10Mb 2ms DropTail
		# Server attached to all clients, but its parent cache is e0
		# delay to server is proportional to its distance to e0
		set delay [expr 5 + $i*5]ms
		$ns duplex-link $node_(e$i) $node_(s0) 1.5Mb $delay DropTail
		set qmon_(svr_f$i) [$ns monitor-queue $node_(s0) $node_(e$i) ""]
		set qmon_(svr_t$i) [$ns monitor-queue $node_(e$i) $node_(s0) ""]
	}
	$self checkConfig $class $ns
}

#
# Simple 2 node topology testing SimpleTcp and TcpApp
#
Class Topology/2node -superclass SkelTopology

Topology/2node instproc init { ns } {
	$self next
	$self instvar node_

	set node_(0) [$ns node]
	set node_(1) [$ns node]
	$ns duplex-link $node_(0) $node_(1) 1.5Mb 10ms DropTail
	$ns duplex-link-op $node_(0) $node_(1) orient right
	$self checkConfig $class $ns
}

#
# 3 node linear topology testing SimpleTcp and TcpApp
#
Class Topology/3node -superclass SkelTopology

Topology/3node instproc init { ns } {
	$self next 
	$self instvar node_

	set node_(0) [$ns node]
	set node_(1) [$ns node]
	set node_(2) [$ns node]
	$ns duplex-link $node_(0) $node_(1) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(1) $node_(2) 1.5Mb 50ms DropTail
	$ns duplex-link-op $node_(0) $node_(1) orient right
	$ns duplex-link-op $node_(1) $node_(2) orient right
}

#
# 5 node topology testing HTTP cache, with 3 clients, one server and 
# one cache
#
Class Topology/5node -superclass SkelTopology

Topology/5node instproc init { ns } {
	$self next
	$self instvar node_

	for {set i 0} {$i < 5} {incr i} {
		set node_($i) [$ns node]
	}
	$ns duplex-link $node_(3) $node_(4) 1Mb 50ms DropTail
	$ns duplex-link $node_(0) $node_(3) 1Mb 50ms DropTail
	$ns duplex-link $node_(1) $node_(3) 1Mb 50ms DropTail
	$ns duplex-link $node_(2) $node_(3) 1Mb 50ms DropTail

	$ns duplex-link-op $node_(4) $node_(3) orient right
	$ns duplex-link-op $node_(0) $node_(3) orient down
	$ns duplex-link-op $node_(1) $node_(3) orient left
	$ns duplex-link-op $node_(2) $node_(3) orient up
}


#----------------------------------------------------------------------
# Section 1: Base test class
#----------------------------------------------------------------------
Class Test

Test instproc init-instvar v {
	set cl [$self info class]
	while { "$cl" != "" } {
		foreach c $cl {
			if ![catch "$c set $v" val] {
				$self set $v $val
				return
			}
		}
		set parents ""
		foreach c $cl {
			if { $cl != "Object" } {
				set parents "$parents [$c info superclass]"
			}
		}
		set cl $parents
	}
}

Test instproc init {} {
	$self instvar ns_ trace_ net_ defNet_ testName_ node_ test_ topo_

	set ns_ [new Simulator -multicast on]

	set cls [$self info class]
	set cls [split $cls /]
	set test_ [lindex $cls [expr [llength $cls] - 1]]

	global opts
	ns-random $opts(ns-random-seed)

	if $opts(nam-trace-all) {
		#set trace_ [open "$test_" w]
		# test-all-template1 requires data file to be temp.rands :(
		set trace_ [open "temp.rands" w]
		$ns_ trace-all $trace_
	}

	if ![info exists opts(net)] {
		set net_ $defNet_
	} else {
		set net_ $opts(net)
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}

	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}

	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}
}

# Use this so derived class would have a chance to overwrite the default net
# of parent classes
Test instproc set-defnet { defnet } {
	$self instvar defNet_
	if ![info exists defNet_] {
		set defNet_ $defnet
	}
}

Test instproc inherit-set { name val } {
	$self instvar $name
	if ![info exists $name] {
		set $name $val
	}
}

Test instproc write-testconf { file } {
	$self instvar test_ net_
	puts $file "# TESTNAME: $test_"
	puts $file "# TOPOLOGY: $net_"
	global opts
	foreach n [lsort [array names opts]] {
		# XXX Remove this after validating existing traces
		if {$n == "quiet"} { continue }
		puts $file "# $n: $opts($n)" 
	}
}

Test instproc set-routing {} {
}

Test instproc set-members {} {
}

Test instproc finish {} {
	$self instvar ns_ trace_

	if [info exists trace_] {
		$ns_ flush-trace
		close $trace_
	}
	exit 0
}

Test instproc run {} {
	$self instvar finishTime_ ns_ trace_

	global opts
	if $opts(nam-trace-all) {
		$self write-testconf $trace_
	}

	$self set-routing
	$self set-members
	$ns_ at $finishTime_ "$self finish"
	$ns_ run
}

# option processing copied from John's ~ns/tcl/ex/rbp_demo.tcl
proc default_options {} {
	global opts opt_wants_arg raw_opt_info

	# raw_opt_info can be set in user's script

	while {$raw_opt_info != ""} {
		if {![regexp "^\[^\n\]*\n" $raw_opt_info line]} {
			break
		}
		regsub "^\[^\n\]*\n" $raw_opt_info {} raw_opt_info
		set line [string trim $line]
		if {[regexp "^\[ \t\]*#" $line]} {
			continue
		}
		if {$line == ""} {
			continue
		} elseif [regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value] {
			set opts($key) $value
			set opt_wants_arg($key) 1
		} elseif [regexp {^([^ ]+)[ ]*$} $line dummy key] {
			# So we don't need to assign opt($key)
			set opt_wants_arg($key) 1
		} else {
			set opt_wants_arg($key) 0
			error "unknown stuff \"$line\" in raw_opt_info"
		}
	}
}

proc process_args {} {
	global argc argv opts opt_wants_arg

	default_options
	for {set i 0} {$i < $argc} {incr i} {
		set key [lindex $argv $i]
		if {$key == "-?" || $key == "--help" || $key == "-help" || $key == "-h"} {
			usage
		}
		regsub {^--} $key {} key
		if {![info exists opt_wants_arg($key)]} {
			#puts stderr "unknown option $key";
			#usage
			continue
		}
		if {$opt_wants_arg($key)} {
			incr i
			set opts($key) [lindex $argv $i]
		} else {
			set opts($key) [expr !opts($key)]
		}
	}
}

# Startup procedure, called at the end of the script
proc run {} {
	global argc argv opts raw_opt_info

	# We don't actually have any real arguments, but we do have 
	# various initializations, which the script depends on.
	process_args
	#set prot $opts(prot)

	# Calling convention by test-all-template1: 
	# ns <file> <test> [QUIET]
	set prot [lindex $argv 0]
	set opts(prot) $prot
	if {$argc > 1} {
		set opts(quiet) 1
	} else {
		set opts(quiet) 0
	}
	set test [new Test/$prot]
	$test run
}


#----------------------------------------------------------------------
# Section 2 Base class for cache testing
#----------------------------------------------------------------------

Class Test-Cache -superclass Test

# Page lifetime is a uniform distribution in [min, max].
Test-Cache set startTime_ 10

Test-Cache instproc init {} {
	$self next
	$self instvar startTime_
	set startTime_ [$class set startTime_]
	$self set-pagepool

	global opts
	if [info exists opts(hb-interval)] {
		Http/Client set hb_interval_ $opts(hb-interval)
		Http/Cache/Inval/Mcast set hb_interval_ $opts(hb-interval)
		Http/Server/Inval/Yuc set hb_interval_ $opts(hb-interval)
	}
	if [info exists opts(upd-interval)] {
		Http/Cache/Inval/Mcast set upd_interval_ $opts(upd-interval)
	}
	if [info exists opts(cache-ims-size)] {
		Http set IMSSize_ $opts(cache-ims-size)
	}
	if [info exists opt(server-inv-size)] {
		Http set INVSize_ $opt(server-inv-size)
	}
	if [info exists opts(cache-ref-size)] {
		Http set REFSize_ $opts(cache-ref-size)
	}
	if [info exists opts(client-req-size)] {
		Http set REQSize_ $opts(client-req-size)
	}
	
	$self instvar ns_
	$ns_ color 40 red
	$ns_ color 41 orange

	# Set default transport to SimpleTcp
	Http set TRANSPORT_ SimpleTcp
}

# Allow global options to preempt, and derived classes to overwrite.
Test-Cache instproc set-server-type { servertype } {
	$self instvar serverType_ 
	global opts
	if [info exists opts(server)] {
		set serverType_ $opts(server)
	} else {
		set serverType_ $servertype
	}
}

Test-Cache instproc set-cache-type { cachetype } {
	$self instvar cacheType_ 
	global opts
	if [info exists opts(cache)] {
		set cacheType_ $opts(cache)
	} else {
		set cacheType_ $cachetype
	}
}

Test-Cache instproc set-client-type { clienttype } {
	$self instvar clientType_
	global opts
	if [info exists opts(client)] {
		set clientType_ $opts(client)
	} else {
		set clientType_ $clienttype
	}
}

Test-Cache instproc set-pagepool {} {
	$self instvar startTime_ finishTime_ pgp_
	global opts
	if [info exists opts(page-file)] {
		set pgp_ [new PagePool/Trace $opts(page-file)]
		set max [$pgp_ get-poolsize]
		set tmp [new RandomVariable/Uniform]
		$tmp set min_ 0
		$tmp set max_ [expr $max - 1]
		$pgp_ ranvar $tmp

		$pgp_ set start_time_ $startTime_
		set finishTime_ [expr [$pgp_ get-duration] + $startTime_]
	} else {
		# Use PagePool/Math
		set pgp_ [new PagePool/Math]
		# Size generator
		set tmp [new RandomVariable/Constant]
		$tmp set val_ $opts(avg-page-size)
		$pgp_ ranvar-size $tmp

		# Age generator
		$self instvar ageRNG_
		if ![info exists ageRNG_] {
			set ageRNG_ [new RNG]
			$ageRNG_ seed $opts(ns-random-seed)
		}
		set tmp [new RandomVariable/Exponential]
		$tmp use-rng $ageRNG_
		$tmp set avg_ $opts(avg-page-age)
		$pgp_ ranvar-age $tmp

		$pgp_ set start_time_ $startTime_
		set finishTime_ [expr $startTime_ + $opts(duration)]
	}
#	puts "Start at $startTime_, stop at $finishTime_"
}

Test-Cache instproc set-req-generator { client } {
	$self instvar pgp_ reqRNG_

	global opts
	if ![info exists reqRNG_] {
		set reqRNG_ [new RNG]
		$reqRNG_ seed $opts(ns-random-seed)
	}
	set tmp [new RandomVariable/Exponential]
	$tmp use-rng $reqRNG_
	$tmp set avg_ $opts(avg-req-interval)
	$client set-interval-generator $tmp
	$client set-page-generator $pgp_
}

Test-Cache instproc create-members {} {
	$self instvar client_ server_ cache_ log_ test_ pgp_ node_ ns_ \
		serverType_ cacheType_ clientType_

	set st $serverType_
	set ct $cacheType_
	set lt $clientType_

	global opts
	if $opts(enable-log) {
		set log_ [open "$test_.log" w]
		$self write-testconf $log_
	}

	foreach n [array names node_] {
		set type [string range $n 0 0]
		set num [string range $n 1 end]
		if {$num == ""} {
			set num 0
		}
		switch $type {
			s {
			 set server_($num) [new Http/Server$st $ns_ $node_($n)]
			 $server_($num) set-page-generator $pgp_
			 if $opts(enable-log) {
				 $server_($num) log $log_
			 }
			}
			e {
			 set cache_($num) [new Http/Cache$ct $ns_ $node_($n)]
			 if $opts(enable-log) {
				 $cache_($num) log $log_
			 }
			}
			c {
			  set client_($num) [new Http/Client$lt $ns_ $node_($n)]
			  $self set-req-generator $client_($num)
			  if $opts(enable-log) {
				  $client_($num) log $log_
			  }
			}
		}
	}
}

Test-Cache instproc set-routing {} {
	$self instvar ns_ mh_
	set mh_ [$ns_ mrtproto CtrMcast {}]
	$ns_ rtproto Session
}

Test-Cache instproc set-members {} {
	$self instvar ns_ finishTime_ startTime_
	$ns_ at $startTime_ "$self start-connection"
#	$ns_ at $finishTime_ "$self finish-connection"
}

Test-Cache instproc set-groups {} {
	# Dummy proc
}

Test-Cache instproc start-connection {} {
	$self instvar ns_

	$self create-members
	$self set-connections
	$self set-groups

	# Let initializations settles down, then start requests
	$ns_ at [expr [$ns_ now] + 10] "$self start-requests"
}

# Empty
Test-Cache instproc set-groups {} {
}

# Empty
Test-Cache instproc set-connections {} {
}

Test-Cache instproc finish {} {
	$self instvar log_
	if [info exists log_] {
		close $log_
	}
	$self next
}


#----------------------------------------------------------------------
# Section 3: 
# Tests of transport protocols and application data transmission over TCP
#----------------------------------------------------------------------

#
# Test SimpleTcp
#
Class Test/SimpleTcp -superclass Test

Test/SimpleTcp instproc init {} {
	$self set-defnet 2node
	$self next
	$self instvar startTime_ finishTime_
	set startTime_ 10
	set finishTime_ 20
	Http set TRANSPORT_ SimpleTcp
}

Test/SimpleTcp instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Test/SimpleTcp instproc set-members {} {
	$self instvar ns_ src_ dst_ node_ ftp1_

	$ns_ at 1.0 "$self start-connection 0 1"
	$ns_ at 9.0 "$self finish-connection 0 1"
}

# Connect TCP source and destination after simulator starts
Test/SimpleTcp instproc start-connection { s d } {
	$self instvar ns_ src_ dst_ node_

	set src_ [new Agent/TCP/SimpleTcp]
	set dst_ [new Agent/TCP/SimpleTcp]
	$src_ set fid_ 0
	$dst_ set fid_ 0
	$ns_ attach-agent $node_($s) $src_
	$ns_ attach-agent $node_($d) $dst_
	$ns_ connect $src_ $dst_ 

	$src_ set dst_addr_ [$dst_ set agent_addr_]
	$src_ set dst_port_ [$dst_ set agent_port_]
	$src_ set window_ 100
	$dst_ listen
	$ns_ at [expr [$ns_ now] + 1.0] "$src_ send 1000"
	$ns_ at [expr [$ns_ now] + 3.0] "$dst_ send 100"
}

Test/SimpleTcp instproc finish-connection { s d } {
	$self instvar ns_ src_ dst_ node_
	$src_ close
}

#
# Base class for testing TcpApp over SimpleTcp and FullTcp
#
Class Test-TcpApp -superclass Test

Test-TcpApp instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Class Test/TcpApp-2node -superclass Test-TcpApp

Test/TcpApp-2node instproc init {} {
	$self set-defnet 2node
	$self next
	$self instvar startTime_ finishTime_ ns_
	set startTime_ 10
	set finishTime_ 50
	$ns_ color 1 red
	$ns_ color 2 blue
}

Test/TcpApp-2node instproc send1 {} {
	$self instvar app1_ app2_
	$app1_ send 40 "$app2_ recv1 40"
}

Test/TcpApp-2node instproc send2 {} {
	$self instvar app1_ app2_ ns_
	$app2_ send 1024 "$app1_ recv2 1024"
	$ns_ at [expr [$ns_ now] + 1.0] "$self send2"
}

Application/TcpApp instproc recv1 { sz } {
	set now [[Simulator instance] now]
	#puts "$now app2 receives data $sz bytes from app1"
}

Application/TcpApp instproc recv2 { sz } {
	set now [[Simulator instance] now]
	#puts "$now app1 receives data $sz bytes from app1"
}

Test/TcpApp-2node instproc set-members {} { 
	$self instvar app1_ app2_ ns_ node_

	set tcp1 [new Agent/TCP/FullTcp]
	set tcp2 [new Agent/TCP/FullTcp]
	$tcp1 set window_ 100
	$tcp1 set fid_ 1
	$tcp2 set window_ 100
	$tcp2 set fid_ 2
	$tcp2 set iss_ 1224
	$ns_ attach-agent $node_(0) $tcp1
	$ns_ attach-agent $node_(1) $tcp2
	$ns_ connect $tcp1 $tcp2
	$tcp2 listen

	set app1_ [new Application/TcpApp $tcp1]
	set app2_ [new Application/TcpApp $tcp2]
	$app1_ connect $app2_

	$ns_ at 1.0 "$self send1"
	$ns_ at 1.2 "$self send2"
}


#----------------------------------------------------------------------
# Section 4: Tests of Cache
#----------------------------------------------------------------------

#
# test simplest http setup: one client + one server
#
Class Test/http1 -superclass Test

Test/http1 instproc init {} {
	$self set-defnet 3node
	$self next
	$self instvar finishTime_ 
	set finishTime_ 40

	# Use simple tcp agent
	Http set TRANSPORT_ SimpleTcp
}

Test/http1 instproc set-members {} {
	$self instvar ns_ src_ dst_ node_ ftp1_

#	set ftp1_ [$src_ attach-app FTP]
	$ns_ at 1.0 "$self start-connection 1 0"
	$ns_ at 9.0 "$self finish-connection 1 0"
	$ns_ at 10.0 "$self start-connection 1 2"
	$ns_ at 19.0 "$self finish-connection 1 2"
}

# Connect TCP source and destination after simulator starts
Test/http1 instproc start-connection { s d } {
	$self instvar ns_ src_ dst_ node_

	set src_ [new Http/Client $ns_ $node_($s)]
	set dst_ [new Http/Server $ns_ $node_($d)]
	$src_ connect $dst_
	$src_ send-request $dst_ GET $dst_:1
}

Test/http1 instproc finish-connection { s d } {
	$self instvar ns_ src_ dst_ node_

	$src_ disconnect $dst_
}

Test/http1 instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Class Test/http1f -superclass Test/http1

Test/http1f instproc init args {
	eval $self next $args
	Http set TRANSPORT_ FullTcp
}


#
# Testing HTTP with one cache, one client and one server
#
Class Test/http2 -superclass Test

Test/http2 instproc init {} {
	$self set-defnet 3node
	$self next
	$self instvar finishTime_ 
	set finishTime_ 40

	Http set TRANSPORT_ SimpleTcp
}

Test/http2 instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Test/http2 instproc set-members {} {
	$self instvar ns_ node_ client_ cache_ server_

	set client_ [new Http/Client $ns_ $node_(0)]
	set cache_ [new Http/Cache $ns_ $node_(1)]
	set server_ [new Http/Server $ns_ $node_(2)]

	$ns_ at 1.0 "$self start-connection"
	$ns_ at 9.0 "$self finish-connection"
	$ns_ at 21.0 "$self start-connection"
	$ns_ at 29.0 "$self finish-connection"
}

# Connect TCP source and destination after simulator starts
Test/http2 instproc start-connection {} {
	$self instvar ns_ client_ server_ cache_ node_

	$client_ connect $cache_
	$cache_ connect $server_
	$cache_ set-parent $server_
	$client_ send-request $cache_ GET $server_:1 
}

Test/http2 instproc finish-connection {} {
	$self instvar client_ server_ cache_

	$client_ disconnect $cache_
	$cache_ disconnect $server_
}

Class Test/http2f -superclass Test/http2

Test/http2f instproc init args {
	eval $self next $args
	Http set TRANSPORT_ FullTcp
}


#----------------------------------------------------------------------
# Testing HTTP with one cache, multiple client and one server
#----------------------------------------------------------------------
Class Test/http3 -superclass Test

Test/http3 instproc init {} {
	$self set-defnet 5node
	$self next
	$self instvar finishTime_ 
	set finishTime_ 40

	Http set TRANSPORT_ SimpleTcp
}

Test/http3 instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Test/http3 instproc set-members {} {
	$self instvar ns_ client_ cache_ server_ node_ test_

	set client_(0) [new Http/Client $ns_ $node_(0)]
	set client_(1) [new Http/Client $ns_ $node_(1)]
	set client_(2) [new Http/Client $ns_ $node_(2)]
	set cache_ [new Http/Cache $ns_ $node_(3)]
	set server_ [new Http/Server $ns_ $node_(4)]

	$ns_ at 1.0 "$self start-connection"
	$ns_ at 9.0 "$self finish-connection"

	# XXX
	# 
	# (1) If we set connection restarts time to 10.0, then we may
	# have a request sent out at 10.0 *before* the connection is 
	# actually re-established, which will result in the lose of a 
	# request packet and the blocking of subsequent requests.
	# 
	# (2) Currently when a connection is shut down, we do *NOT* 
	# clean up pending requests. This will result in the possible
	# blocking of requests after the connection is re-established. 
	# This test illustrates this effect.
	# 
	# The cleaning of a cache after disconnection is currently *NOT*
	# implemented. It can be disconnected but its behavior after
	# re-connection is not defined. NOTE: disconnection means 
	# explicitly call Http::disconnect(). Link dynamics and losses 
	# are supported.

	$ns_ at 9.9 "$self start-connection"
	$ns_ at 19.0 "$self finish-connection"
}

# Connect TCP source and destination after simulator starts
Test/http3 instproc start-connection {} {
	$self instvar ns_ client_ server_ cache_ node_

	$client_(0) connect $cache_
	$client_(1) connect $cache_
	$client_(2) connect $cache_
	$cache_ connect $server_
	$cache_ set-parent $server_
	$self start-request
}

Test/http3 instproc start-request {} {
	$self instvar client_ ns_ cache_ server_
	$client_(0) send-request $cache_ GET $server_:0
	set tmp [expr [$ns_ now] + 1]
	$ns_ at $tmp "$client_(1) send-request $cache_ GET $server_:1"
	set tmp [expr $tmp + 1]
	$ns_ at $tmp "$client_(2) send-request $cache_ GET $server_:0"
	set tmp [expr $tmp + 2]
	$ns_ at $tmp "$self start-request"
}

Test/http3 instproc finish-connection {} {
	$self instvar client_ server_ cache_

	$client_(0) disconnect $cache_
	$client_(1) disconnect $cache_
	$client_(2) disconnect $cache_
	$cache_ disconnect $server_
}

Class Test/http3f -superclass Test/http3

Test/http3f instproc init args {
	eval $self next $args
	Http set TRANSPORT_ FullTcp
}


#
# Testing cache with TTL invalidation
#
Class Test/http4 -superclass Test

Test/http4 instproc init {} {
	$self set-defnet 5node
	$self next
	$self instvar ns_ startTime_ finishTime_ 
	set startTime_ 1
	set finishTime_ 40

	Http set TRANSPORT_ SimpleTcp
}

Test/http4 instproc set-routing {} {
	$self instvar ns_
	$ns_ rtproto Session
}

Test/http4 instproc set-topology {} {
	$self instvar node_ ns_

	for {set i 0} {$i < 5} {incr i} {
		set node_($i) [$ns_ node]
	}
	$ns_ duplex-link $node_(3) $node_(4) 1Mb 50ms DropTail
	$ns_ duplex-link $node_(0) $node_(3) 1Mb 50ms DropTail
	$ns_ duplex-link $node_(1) $node_(3) 1Mb 50ms DropTail
	$ns_ duplex-link $node_(2) $node_(3) 1Mb 50ms DropTail
}

Test/http4 instproc set-members {} {
	$self instvar ns_ startTime_ client_ cache_ server_ node_ test_

	set client_(0) [new Http/Client $ns_ $node_(0)]
	set client_(1) [new Http/Client $ns_ $node_(1)]
	set client_(2) [new Http/Client $ns_ $node_(2)]
	set cache_ [new Http/Cache/TTL $ns_ $node_(3)]
	set server_ [new Http/Server $ns_ $node_(4)]

	$ns_ at $startTime_ "$self start-connection"
	$ns_ at 10 "$self finish-connection"
}

Test/http4 instproc start-requests {} {
	$self instvar client_ server_ cache_ ns_

	$client_(0) send-request $cache_ GET $server_:0
	set tmp [expr [$ns_ now] + 1]
	$ns_ at $tmp "$client_(1) send-request $cache_ GET $server_:1"
	incr tmp
	$ns_ at $tmp "$client_(2) send-request $cache_ GET $server_:0"
	incr tmp 3
	$ns_ at $tmp "$self start-requests"
}

# Connect TCP source and destination after simulator starts
Test/http4 instproc start-connection {} {
	$self instvar ns_ client_ server_ cache_ node_ 

	$client_(0) connect $cache_
	$client_(1) connect $cache_
	$client_(2) connect $cache_
	$cache_ connect $server_
	$cache_ set-parent $server_
	$self start-requests
}

Test/http4 instproc finish-connection {} {
	$self instvar client_ server_ cache_

	$client_(0) disconnect $cache_
	$client_(1) disconnect $cache_
	$client_(2) disconnect $cache_
	$cache_ disconnect $server_
}

Class Test/http4f -superclass Test/http4

Test/http4f instproc init args {
	eval $self next $args
	Http set TRANSPORT_ FullTcp
}


#
# Testing PagePool
#
Class Test/PagePool -superclass Test

Test/PagePool instproc init {} {
	$self instvar pgp_ 
	global opts
	set opts(page-file) pages
	set pgp_ [new PagePool/Trace $opts(page-file)]
	set max [$pgp_ get-poolsize]
	set tmp [new RandomVariable/Uniform]
	$tmp set min_ 0
	$tmp set max_ [expr $max - 1]
	$pgp_ ranvar $tmp
}

Test/PagePool instproc test-enumerate {} { 
	$self instvar pgp_ log_
	set max [$pgp_ get-poolsize]
	for {set i 0} {$i < $max} {incr i} {
		puts -nonewline $log_ "Page $i: "
		puts -nonewline $log_ "size [$pgp_ gen-size $i] "
		set mtime [$pgp_ gen-modtime $i -1]
		puts -nonewline $log_ "ctime $mtime "
		set tmp [$pgp_ gen-modtime $i $mtime]
		while {$tmp != $mtime} {
			puts -nonewline $log_ "mtime $tmp "
			set mtime $tmp
			set tmp [$pgp_ gen-modtime $i $mtime]
		}
		puts $log_ ""
	}
}

Test/PagePool instproc test-getpageid {} {
	$self instvar pgp_ log_
	set max [$pgp_ get-poolsize]
	for {set i 0} {$i < $max} {incr i} {
		set id [$pgp_ gen-pageid 0]
		puts -nonewline $log_ "Page $id: "
		puts -nonewline $log_ "size [$pgp_ gen-size $id] "
		set mtime [$pgp_ gen-modtime $id -1]
		puts -nonewline $log_ "ctime $mtime "
		set tmp [$pgp_ gen-modtime $id $mtime]
		while {$tmp != $mtime} {
			puts -nonewline $log_ "mtime $tmp "
			set mtime $tmp
			set tmp [$pgp_ gen-modtime $id $mtime]
		}
		puts $log_ ""
	}
}

Test/PagePool instproc run {} {
	$self instvar log_
	set log_ [open "temp.rands" w]
	$self test-getpageid
	$self test-enumerate
	close $log_
}


#----------------------------------------------------------------------
# Testing simplest case for heartbeat message: 1 client+1 cache+1 server
#----------------------------------------------------------------------

# Multicast invalidation + server invalidation
Class Test/cache0-inv -superclass Test-Cache

Test/cache0-inv instproc init {} {
	$self set-defnet cache0
	$self next

	$self set-server-type /Inval/Yuc
	$self set-cache-type /Inval/Mcast
	$self set-client-type ""
	Http set TRANSPORT_ SimpleTcp
}

Test/cache0-inv instproc set-connections {} {
	$self instvar client_ server_ cache_ 

	# XXX Should always let server connects to cache first, then requests
	$client_(0) connect $cache_(0)
	$server_(0) connect $cache_(0)
	$server_(0) set-parent-cache $cache_(0)
}

Test/cache0-inv instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_
	$client_(0) start $cache_(0) $server_(0)
}

# Mcast inval
Class Test/cache0f-inv -superclass Test/cache0-inv

Test/cache0f-inv instproc init args {
	eval $self next $args
	Http set TRANSPORT_ FullTcp
}

# Push + mcast inval
Class Test/cache0-push -superclass Test/cache0-inv

Test/cache0-push instproc create-members {} {
	$self next
	$self instvar cache_ server_
	$server_(0) set enable_upd_ 1
	$cache_(0) set enable_upd_ 1
}

Class Test/cache0f-push -superclass {Test/cache0-push Test/cache0f-inv}

# TTL 
Class Test/cache0-ttl -superclass Test/cache0-inv

Test/cache0-ttl instproc init args {
	eval $self next $args
	$self set-server-type ""
	$self set-cache-type /TTL
	$self set-client-type ""
}

Test/cache0-ttl instproc set-connections {} {
	$self instvar client_ server_ cache_ 
	# XXX Should always let server connects to cache first, then requests
	$client_(0) connect $cache_(0)
	$cache_(0) connect $server_(0)
	$server_(0) set-parent-cache $cache_(0)
}

Class Test/cache0f-ttl -superclass {Test/cache0f-inv Test/cache0-ttl}

# Omniscient TTL
Class Test/cache0-ottl -superclass Test/cache0-ttl

Test/cache0-ottl instproc init args {
	eval $self next $args
	$self set-cache-type /TTL/Omniscient
}

Class Test/cache0f-ottl -superclass {Test/cache0-ottl Test/cache0f-ttl}


#----------------------------------------------------------------------
# Two hierarchies #1: server0 -> root cache 0
#----------------------------------------------------------------------
Class Test/TLC1 -superclass Test-Cache

Test/TLC1 instproc init {} {
	# Do our own initialization
	global opts
	set opts(duration) 500
	set opts(avg-page-age) 60
	set opts(avg-req-interval) 6
	set opts(hb-interval) 6

	$self set-defnet cache2
	$self next
	$self set-cache-type /Inval/Mcast 
	$self set-server-type /Inval/Yuc
	$self set-client-type ""
	Http set TRANSPORT_ SimpleTcp
}

Test/TLC1 instproc start-requests {} {
	$self instvar client_ cache_ server_

	$client_(0) start $cache_(2) $server_(0)
	$client_(1) start $cache_(6) $server_(0)
	$client_(2) start $cache_(4) $server_(0)
	$client_(3) start $cache_(1) $server_(0)
}

Test/TLC1 instproc set-connections {} {
	$self instvar client_ cache_ server_

	$client_(0) connect $cache_(2)
	$client_(1) connect $cache_(6)
	$client_(2) connect $cache_(4)
	$client_(3) connect $cache_(1)
	$cache_(2) connect $cache_(0)
	$cache_(2) set-parent $cache_(0)
	$cache_(3) connect $cache_(0)
	$cache_(3) set-parent $cache_(0)
	$cache_(6) connect $cache_(2)
	$cache_(6) set-parent $cache_(2)
	$cache_(4) connect $cache_(1)
	$cache_(4) set-parent $cache_(1)
	$cache_(5) connect $cache_(1)
	$cache_(5) set-parent $cache_(1)

	# XXX
	# We also need TCP connections between TLCs, but the order in which
	# they are connected is tricky. I.e., the cache that first sends 
	# out a packet should connect first. But how do we know which cache
	# would send out a packet first???
	$cache_(1) connect $cache_(0)
}

Test/TLC1 instproc set-groups {} {
	$self instvar client_ cache_ server_ mh_

	# TBA group setup stuff...
	set grp [Node allocaddr]
	$cache_(0) join-tlc-group $grp
	$cache_(1) join-tlc-group $grp
	$mh_ switch-treetype $grp

	set grp [Node allocaddr]
	$cache_(0) init-inval-group $grp
	$cache_(2) join-inval-group $grp
	$cache_(3) join-inval-group $grp
	$mh_ switch-treetype $grp

	set grp [Node allocaddr]
	$cache_(1) init-inval-group $grp
	$cache_(4) join-inval-group $grp
	$cache_(5) join-inval-group $grp
	$mh_ switch-treetype $grp

	set grp [Node allocaddr]
	$cache_(2) init-inval-group $grp
	$cache_(6) join-inval-group $grp
	$mh_ switch-treetype $grp

	# XXX Must let the server to initialize connection, because it's 
	# going to send out the first packet
	$cache_(1) connect $server_(0)
	$server_(0) connect $cache_(0)

	# XXX Must do this at the end. It'll trigger a lot of JOINs.
	$server_(0) set-parent-cache $cache_(0)
	# XXX Must do this when using multiple hierarchies
	$server_(0) set-tlc $cache_(0)
}

Class Test/TLC1f -superclass Test/TLC1

Test/TLC1f instproc init {} {
	$self next
	Http set TRANSPORT_ FullTcp
}

#
# Two hierarchies with direct request
#
#Class Test/TLC1-dreq -superclass Test/TLC1

# Test/TLC1-dreq instproc init {} {
# 	$self next
# 	$self set-cache-type /Inval/Mcast/Perc
# }

# Set up direct connections from leaf caches (i.e., all caches who 
# may connect to a browser) to the server
# Test/TLC1-dreq instproc set-connections {} {
# 	$self next
# 	$self instvar cache_ server_
# 	$cache_(1) connect $server_(0)
# 	$cache_(2) connect $server_(0)
# 	$cache_(4) connect $server_(0)
# 	$cache_(6) connect $server_(0)
# 	$cache_(1) set direct_request_ 1
# 	$cache_(2) set direct_request_ 1
# 	$cache_(4) set direct_request_ 1
# 	$cache_(6) set direct_request_ 1
# }



#----------------------------------------------------------------------
# Testing server/cache liveness messages and failure recovery
#----------------------------------------------------------------------
Class Test/Liveness -superclass Test-Cache

Test/Liveness instproc init {} {
	# Set default initialization values
	global opts
	set opts(duration) 1200	;# Link heals at time 1000.
	set opts(avg-page-age) 60
	set opts(avg-req-interval) 60
	set opts(hb-interval) 30
	
	$self set-defnet cache4d
	$self next
	$self set-cache-type /Inval/Mcast
	$self set-server-type /Inval/Yuc
	$self set-client-type ""

	# Must use FullTcp, because we'll have packet loss, etc.
	Http set TRANSPORT_ FullTcp
}

Test/Liveness instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_

	$client_(0) start $cache_(3) $server_(0)
	$client_(1) start $cache_(4) $server_(0)
	$client_(2) start $cache_(5) $server_(0)
	$client_(3) start $cache_(6) $server_(0)
#	puts "At [$ns_ now], request starts"
}

Test/Liveness instproc set-connections {} {
	$self instvar ns_ client_ server_ cache_ 

	# Enable dynamics somewhere
	$client_(0) connect $cache_(3)
	$client_(1) connect $cache_(4)
	$client_(2) connect $cache_(5)
	$client_(3) connect $cache_(6)

	$cache_(1) connect $cache_(0)
	$cache_(2) connect $cache_(0)
	$cache_(3) connect $cache_(1)
	$cache_(4) connect $cache_(1)
	$cache_(5) connect $cache_(2)
	$cache_(6) connect $cache_(2)
	$cache_(1) set-parent $cache_(0)
	$cache_(2) set-parent $cache_(0)
	$cache_(3) set-parent $cache_(1)
	$cache_(4) set-parent $cache_(1)
	$cache_(5) set-parent $cache_(2)
	$cache_(6) set-parent $cache_(2)

	# All TLCs have connection to server
	$cache_(0) connect $server_(0)
	# Parent cache of the server is e3
	$server_(0) connect $cache_(3)
}

Test/Liveness instproc set-groups {} {
	$self instvar cache_ mh_ server_

	set grp [Node allocaddr]
	$cache_(0) init-inval-group $grp
	$cache_(1) join-inval-group $grp
	$cache_(2) join-inval-group $grp
	$mh_ switch-treetype $grp

	set grp [Node allocaddr]
	$cache_(1) init-inval-group $grp
	$cache_(3) join-inval-group $grp
	$cache_(4) join-inval-group $grp
	$mh_ switch-treetype $grp

	set grp [Node allocaddr]
	$cache_(2) init-inval-group $grp
	$cache_(5) join-inval-group $grp
	$cache_(6) join-inval-group $grp
	$mh_ switch-treetype $grp

	$server_(0) set-parent-cache $cache_(3)
}


#----------------------------------------------------------------------
# Test Group 1: 
#
# Poisson page mods and Poisson requests, one bottleneck link, 2-level 
# cache hierarchy with a single TLC. No loss.
#
# Comparing Invalidation, TTL and OTTL.
#
# Testing Mcast+Yucd using a bottleneck topology
#----------------------------------------------------------------------
Class Test/Mcast-PB -superclass Test-Cache

Test/Mcast-PB instproc init {} {
	# Our own initializations
	global opts
	set opts(duration) 200
	set opts(avg-page-age) 10
	set opts(avg-req-interval) 6
	set opts(hb-interval) 6
	set opts(num-2nd-cache) 5

	$self set-defnet BottleNeck
	$self next
	$self instvar secondCaches_
	set secondCaches_ $opts(num-2nd-cache)
	$self set-cache-type /Inval/Mcast
	$self set-server-type /Inval/Yuc
	$self set-client-type ""
}

Test/Mcast-PB instproc start-requests {} {
	$self instvar client_ cache_ server_ secondCaches_
	
	set n $secondCaches_
	for {set i 0} {$i < $n} {incr i} {
		$client_($i) start $cache_($i) $server_(0)
	}

	$self instvar pgp_ topo_ ns_

	# Because Test/Cache::init{} already did set-pagepool{}, now we 
	# know how many pages we have. Estimate the cache population time
	# by NumPages*1+10, then start bandwidth monitoring after 
	# the caches are populated with pages
	$ns_ at [expr [$ns_ now] + [$pgp_ get-poolsize] + 10] \
		"$topo_ start-monitor $ns_"
}

Test/Mcast-PB instproc set-connections {} {
	$self instvar ns_ client_ server_ cache_ secondCaches_

	set n $secondCaches_
	for {set i 0} {$i < $n} {incr i} {
		$client_($i) connect $cache_($i)
		$cache_($i) connect $cache_($n)
		$cache_($i) set-parent $cache_($n)
	}
	$cache_($n) connect $server_(0)
	$self connect-server
}

Test/Mcast-PB instproc connect-server {} {
	$self instvar server_ cache_
	$server_(0) connect $cache_(0)
}

Test/Mcast-PB instproc set-groups {} {
	$self instvar cache_ server_ secondCaches_ mh_

	set n $secondCaches_
	set grp1 [Node allocaddr]
	set grp2 [Node allocaddr]
	$cache_($n) init-inval-group $grp1
	$cache_($n) init-update-group $grp2
	for {set i 0} {$i < $n} {incr i} {
		$cache_($i) join-inval-group $grp1
		$cache_($i) join-update-group $grp2
	}
	$mh_ switch-treetype $grp1
	$mh_ switch-treetype $grp2

	$server_(0) set-parent-cache $cache_(0)
}

Test/Mcast-PB instproc collect-stat {} {
	$self instvar topo_ client_ server_ cache_ secondCaches_
	set bw [$topo_ mon-stat]

	set sn 0
	set gn 0
	set sr 0
	set st(max) 0
	set st(min) 98765432
	set st(avg) 0
	set rt(max) 0
	set rt(min) 98765432
	set rt(avg) 0
	foreach c [array names client_] {
		set gn [expr $gn + [$client_($c) stat req-num]]
		set sn [expr $sn + [$client_($c) stat stale-num]]

		set st(avg) [expr $st(avg) + [$client_($c) stat stale-time]]
		set tmp [$client_($c) stat st-min]
		if { $tmp < $st(min) } { set st(min) $tmp }
		set tmp [$client_($c) stat st-max]
		if { $tmp > $st(max) } { set st(max) $tmp }

		set rt(avg) [expr $rt(avg) + [$client_($c) stat rep-time]]
		set tmp [$client_($c) stat rt-max]
		if { $tmp > $rt(max) } { set rt(max) $tmp }
		set tmp [$client_($c) stat rt-min]
		if { $tmp < $rt(min) } { set rt(min) $tmp }
	}
	if {$st(max) < $st(min)} {
		set st(max) 0
		set st(min) 0
	}
	if {$rt(max) < $rt(min)} {
		set rt(max) 0
		set rt(min) 0
	}
	if {$gn > 0} {
		set sr [expr double($sn) / $gn * 100]
	}
	if {$sn > 0} {
		if [catch {set st(avg) [expr double($st(avg)) / $sn]}] {
			set st(avg) 0	;# No stale hits
		} 
	}
	if {$gn > 0} {
		set rt(avg) [expr double($rt(avg)) / $gn]
	}

	set ims 0
	foreach c [array names cache_] {
		set ims [expr $ims + [$cache_($c) stat ims-num]]
	}

	set res [list sr $sr sh [$server_(0) stat hit-num] th [$cache_($secondCaches_) stat hit-num] st $st(avg) st-max $st(max) st-min $st(min) rt $rt(avg) rt-max $rt(max) rt-min $rt(min) mn [$server_(0) stat mod-num] ims-num $ims]
	return [concat $bw $res]
}

Test/Mcast-PB instproc output-stat { args } {
	eval array set d $args
	global opts 
	# XXX Don't have statistics for total bandwidth. :(
	#puts "$opts(hb-interval) Bandwidth*Hop -1 Stale $d(sr) AverageRepTime $d(rt) BottleneckBW $d(btnk_bw) ServerBW $d(svr_bw) StaleTime $d(st)"
}

Test/Mcast-PB instproc finish {} {
	global opts
	if $opts(quiet) {
		$self output-stat [$self collect-stat]
	}
	$self next
}

#
# Same as mcast-PB, except using Inval/Mcast/Perc cache
# 
Class Test/Mcast-PBP -superclass Test/Mcast-PB

Test/Mcast-PBP instproc init {} {
	$self next
	$self set-cache-type /Inval/Mcast/Perc
}

#
# Same as mcast-PB, except enabled selective push of updates
#
Class Test/Mcast-PBU -superclass Test/Mcast-PB

Test/Mcast-PBU instproc create-members {} {
	$self next
	$self instvar cache_ server_
	foreach n [array names cache_] {
		$cache_($n) set enable_upd_ 1
	}
	foreach n [array names server_] {
		$server_($n) set enable_upd_ 1
	}
}

#
# Mcast invalidation + selective push + mandatory push
#
Class Test/Mcast-PBU-MP -superclass Test/Mcast-PBU

Test/Mcast-PBU-MP instproc create-members {} {
	$self next
	$self instvar client_ ns_ server_
	$ns_ at 100.0 "$client_(1) request-mpush $server_(0):0"
	$ns_ at 500.0 "$client_(1) stop-mpush $server_(0):0"
}

#
# Testing TTL using a bottleneck topology
#
Class Test/ttl-PB -superclass Test/Mcast-PB

Test/ttl-PB instproc init {} {
	global opts
	set opts(ttl) 0.1

	$self next
	$self set-cache-type /TTL
	$self set-server-type ""
	$self set-client-type ""
}

Test/ttl-PB instproc create-members {} {
	$self next
	global opts
	$self instvar cache_
	foreach n [array names cache_] {
		$cache_($n) set-thresh $opts(ttl)
	}
}

Test/ttl-PB instproc set-groups {} {
	# We do not set any mcast groups
}

Test/ttl-PB instproc connect-server {} {
	$self instvar server_ cache_
	$cache_(0) connect $server_(0)
}

Test/ttl-PB instproc output-stat { args } {
	eval array set d $args
	global opts 
	# XXX Don't have statistics for total bandwidth. :(
	#puts "$opts(ttl) Bandwidth*Hop -1 Stale $d(sr) AverageRepTime $d(rt) BottleneckBW $d(btnk_bw) ServerBW $d(svr_bw) StaleTime $d(st)"
}

#
# Testing Omniscient TTL using a bottleneck topology
#
Class Test/ottl-PB -superclass {Test/ttl-PB Test/Mcast-PB}

Test/ottl-PB instproc init {} {
	$self next
	$self set-cache-type /TTL/Omniscient
	$self set-server-type ""
	$self set-client-type ""
}

Test/ottl-PB instproc output-stat { args } {
	eval array set d $args
	# XXX Don't have statistics for total bandwidth. :(
	#puts "Bandwidth*Hop -1 Stale $d(sr) AverageRepTime $d(rt) BottleneckBW $d(btnk_bw) ServerBW $d(svr_bw) StaleTime $d(st)"
}


#
# All the above tests with real traces
#
Class Test/Mcast-PBtr -superclass Test/Mcast-PB

Test/Mcast-PBtr instproc init {} {
	$self inherit-set pagepoolType_ "ProxyTrace"
	$self next
	Http set TRANSPORT_ FullTcp
}

Test/Mcast-PBtr instproc populate-cache {} {
	# Populate servers and caches with pages.
	# Do not use Http/Client::populate{}!
	$self instvar pgp_ cache_ server_ secondCaches_ startTime_ ns_
	set n $secondCaches_
	for {set i 0} {$i < [$pgp_ get-poolsize]} {incr i} {
		set pageid $server_(0):$i
		$server_(0) gen-page $pageid
		#set pageinfo [$server_(0) get-page $pageid]
		#for {set j 0} {$j < $secondCaches_} {incr j} {
		#	eval $cache_($j) enter-page $pageid $pageinfo
		#}
		#eval $cache_($secondCaches_) enter-page $pageid $pageinfo
#		if {$i % 1000 == 0} {
#			puts "$i pages populated"
#		}
	}
}

Test/Mcast-PBtr instproc start-connection {} {
	$self next
	$self populate-cache
}

Test/Mcast-PBtr instproc start-requests {} {
	$self instvar client_ cache_ server_ secondCaches_
	
	for {set i 0} {$i < $secondCaches_} {incr i} {
		# Use start-session{} to avoid populating cache
		$client_($i) start-session $cache_($i) $server_(0)
	}
	
	$self instvar topo_ ns_
	$topo_ start-monitor $ns_
}

Test/Mcast-PBtr instproc set-pagepool {} {
	$self instvar startTime_ finishTime_ pgp_ ns_ pagepoolType_
	global opts
	if {![info exists opts(xtrace-req)] || ![info exists opts(xtrace-page)]} {
		error "Must supply request logs and page logs of proxy traces"
	}
	set pgp_ [new PagePool/$pagepoolType_]
	$pgp_ set-reqfile $opts(xtrace-req)
	$pgp_ set-pagefile $opts(xtrace-page)
	$pgp_ bimodal-ratio 0.1
	$pgp_ set-client-num $opts(num-2nd-cache)

	# XXX Do *NOT* set start time of page generators. It'll be set
	# after the cache population phase
	# Estimate a finish time
	set opts(duration) [$pgp_ get-duration]
	set finishTime_ [expr $opts(duration) + $startTime_]
	#puts "Duration changed to $opts(duration), finish at $finishTime_"

	$self instvar ageRNG_
	if ![info exists ageRNG_] {
		set ageRNG_ [new RNG]
		$ageRNG_ seed $opts(ns-random-seed)
	}

	# Dynamic page, with page modification 
	set tmp [new RandomVariable/Uniform]
	$tmp use-rng $ageRNG_
	$tmp set min_ [expr $opts(avg-page-age)*0.001]
	$tmp set max_ [expr $opts(avg-page-age)*1.999]
	$pgp_ ranvar-dp $tmp

	# Static page
	set tmp [new RandomVariable/Uniform]
	$tmp use-rng $ageRNG_
	$tmp set min_ [expr $finishTime_ * 1.1]
	$tmp set max_ [expr $finishTime_ * 1.2]
	$pgp_ ranvar-sp $tmp
}

# Set every client's request generator to pgp_
Test/Mcast-PBtr instproc set-req-generator { client } {
	$self instvar pgp_
	$client set-page-generator $pgp_
}

Class Test/Mcast-PBPtr -superclass {Test/Mcast-PBP Test/Mcast-PBtr}

Class Test/Mcast-PBUtr -superclass {Test/Mcast-PBU Test/Mcast-PBtr}

Class Test/ttl-PBtr -superclass {Test/ttl-PB Test/Mcast-PBtr}

Class Test/ottl-PBtr -superclass {Test/ottl-PB Test/Mcast-PBtr}


#----------------------------------------------------------------------
# Test group 2
#
# Same as test group 1, except using compound pages
#
# Mcast-PB with compound pages
#----------------------------------------------------------------------
Class Test/mmcast-PB -superclass Test/Mcast-PB

Test/mmcast-PB instproc init {} {
	$self next
	$self set-cache-type /Inval/Mcast/Perc
	$self set-server-type /Inval/MYuc
	$self set-client-type /Compound
}

Test/mmcast-PB instproc set-pagepool {} {
	$self instvar startTime_ finishTime_ pgp_
	global opts
	# Use PagePool/Math, which means a single page
	set pgp_ [new PagePool/CompMath]

	# Size generator
	$pgp_ set main_size_ $opts(avg-page-size)
	$pgp_ set comp_size_ $opts(comp-page-size)

	# Age generator
	$self instvar ageRNG_
	if ![info exists ageRNG_] {
		set ageRNG_ [new RNG]
		$ageRNG_ seed $opts(ns-random-seed)
	}
	set tmp [new RandomVariable/Exponential]
	$tmp use-rng $ageRNG_
	$tmp set avg_ $opts(avg-page-age)
	$pgp_ ranvar-main-age $tmp

	# Compound age generator
	$self instvar compAgeRNG_
	if ![info exists compAgeRNG_] {
		set compAgeRNG_ [new RNG]
		$compAgeRNG_ seed $opts(ns-random-seed)
	}
	set tmp [new RandomVariable/Uniform]
	$tmp use-rng $compAgeRNG_
	$tmp set min_ [expr $opts(avg-comp-page-age) * 0.9]
	$tmp set max_ [expr $opts(avg-comp-page-age) * 1.1]
	$pgp_ ranvar-obj-age $tmp

	$pgp_ set num_pages_ [expr $opts(num-comp-pages) + 1]

	$pgp_ set start_time_ $startTime_
	set finishTime_ [expr $startTime_ + $opts(duration)]
#	puts "Start at $startTime_, stop at $finishTime_"
}

#
# selective push + inval
#
Class Test/mmcast-PBU -superclass {Test/Mcast-PBU Test/mmcast-PB}

#
# TTL with compound page
#
Class Test/mttl-PB -superclass {Test/ttl-PB Test/mmcast-PB}

Test/mttl-PB instproc init {} {
	$self next
	$self set-cache-type /TTL
	$self set-server-type /Compound
	$self set-client-type /Compound
}

#
# Omniscient TTL + compound page
#
Class Test/mottl-PB -superclass {Test/ottl-PB Test/mmcast-PB}

Test/mottl-PB instproc init {} {
	$self next
	$self set-cache-type /TTL/Omniscient
	$self set-server-type /Compound
	$self set-client-type /Compound
}


#----------------------------------------------------------------------
# Test group 3
#
# Comparison of direct request+invalidation vs ttl+direct request
#
# Topology is derived from the BottleNeck topology. It adds additional
# direct links from every leaf cache to the web server. This link is
# used to model the "short path" from leaf caches to the server.
#----------------------------------------------------------------------
Class Test-dreq -superclass Test-Cache

Test-dreq instproc init {} {
    $self set-defnet cache5
    $self next
    
    $self instvar secondCaches_
    global opts
    set secondCaches_ $opts(num-2nd-cache)
}

Test-dreq instproc start-requests {} {
	$self instvar client_ server_ cache_ secondCaches_
	for {set i 0} {$i < $secondCaches_} {incr i} {
		$client_($i) start $cache_($i) $server_(0)
	}
}

Test-dreq instproc set-connections {} {
	$self instvar client_ server_ cache_ secondCaches_ ns_
	for {set i 0} {$i < $secondCaches_} {incr i} {
		$client_($i) connect $cache_($i)
	}
}

Test-dreq instproc collect-stat {} {
	$self instvar topo_ client_ secondCaches_
	$topo_ instvar qmon_

	set svr_bw 0
	for {set i 0} {$i < $secondCaches_} {incr i} {
		set svr_bw [expr [$qmon_(svr_f$i) set bdepartures_] + \
			$svr_bw + [$qmon_(svr_t$i) set bdepartures_]]
	}
	set btnk_bw [expr [$qmon_(btnk_f) set bdepartures_] + \
			[$qmon_(btnk_t) set bdepartures_]]
	set sn 0
	set gn 0
	set st 0
	set rt 0
	foreach c [array names client_] {
		set gn [expr $gn + [$client_($c) stat req-num]]
		set sn [expr $sn + [$client_($c) stat stale-num]]
		set st [expr $st + [$client_($c) stat stale-time]]
		set rt [expr $rt + [$client_($c) stat rep-time]]
	}
	if {$gn > 0} {
		set sr [expr double($sn) / $gn * 100]
	}
	if {$sn > 0} {
		if [catch {set st [expr double($st) / $sn]}] {
			set st 0	;# No stale hits
		}
	}
	if {$gn > 0} {
		set rt [expr double($rt) / $gn]
	}
	return [list svr_bw $svr_bw btnk_bw $btnk_bw sr $sr st $st rt $rt]
}

Test-dreq instproc finish {} {
	$self output-stat [$self collect-stat]
	$self next
}

#Class Test/mcast-dreq -superclass Test-dreq

#Test/mcast-dreq instproc init {} {
	# $self next
# 	$self set-cache-type /Inval/Mcast/Perc
# 	$self set-server-type /Inval/Yuc
# 	$self set-client-type ""
# }

# Test/mcast-dreq instproc output-stat { args } {
# 	eval array set d $args
# 	global opts 
# 	# XXX Don't have statistics for total bandwidth. :(
# 	#puts "$opts(hb-interval) Bandwidth*Hop -1 Stale $d(sr) AverageRepTime $d(rt) BottleneckBW $d(btnk_bw) ServerBW $d(svr_bw) StaleTime $d(st)"
# }

# Test/mcast-dreq instproc set-connections {} {
# 	$self next ;# connecting clients

# 	$self instvar server_ cache_ secondCaches_ 
# 	set n $secondCaches_
# 	for {set i 0} {$i < $secondCaches_} {incr i} {
# 		$cache_($i) connect $cache_($n)
# 		$cache_($i) set-parent $cache_($n)
# 		if $i {
# 			# Let all leaf caches connect to server
# 			$cache_($i) connect $server_(0)
# 		}
# 	}
# 	$cache_($n) connect $server_(0)
# 	$server_(0) connect $cache_(0)
# }

# Test/mcast-dreq instproc set-groups {} {
# 	$self instvar cache_ server_ secondCaches_ mh_

# 	set n $secondCaches_
# 	set grp1 [Node allocaddr]
# 	set grp2 [Node allocaddr]
# 	$cache_($n) init-inval-group $grp1
# 	$cache_($n) init-update-group $grp2
# 	for {set i 0} {$i < $n} {incr i} {
# 		$cache_($i) join-inval-group $grp1
# 		$cache_($i) join-update-group $grp2
# 		# Every leaf cache uses direct request
# 		$cache_($i) set direct_request_ 1
# 	}
# 	$mh_ switch-treetype $grp1
# 	$mh_ switch-treetype $grp2

# 	$server_(0) set-parent-cache $cache_(0)
# }


#----------------------------------------------------------------------
# Options 
#----------------------------------------------------------------------
global raw_opt_info
set raw_opt_info {
	# Random number seed; default is 0, so ns will give a 
	# diff. one on each invocation.
	# XXX Get a "good" seed from predef_seeds[] in rng.cc
	ns-random-seed 188312339
		
	# Animation options; complete traces are useful
	# for nam only, so do those only when a tracefile
	# is being used for nam
	nam-trace-all 1

	enable-log 0

	# Tests to be used
	prot

	duration 500
		    
	# Trace file used for PagePool
	page-file 
	
	# TTL threshold
	ttl 0.1
	
	# Cache type
	cache 
	# server type
	server 
	
	# Packet size configurations
	cache-ims-size	50
	cache-ref-size	50
	server-inv-size	43
	client-req-size 43
	
	# request intervals
	min-req-interval 50
	max-req-interval 70
	avg-req-interval 60

	min-page-size 100
	max-page-size 50000
	avg-page-size 1024

	min-page-age  50
	max-page-age  70
	avg-page-age  60

	# compound page size: 50K
	comp-page-size 51200
	avg-comp-page-age 40000
	num-comp-pages 1

	# If we use only one page
	single-page 1

	hb-interval 30
	upd-interval 5

	# Number of second level caches. Needed by Topology/BottleNeck
	num-2nd-cache 5

	scheduler-type Calendar

	# Proxy trace files: requests and pages
	xtrace-req webtrace-reqlog
	xtrace-page webtrace-pglog
}


#----------------------------------------------------------------------
# Execution starts...
#----------------------------------------------------------------------
run
