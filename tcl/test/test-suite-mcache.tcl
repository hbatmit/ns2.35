# Test suite for HTTP server, client, proxy cache.
#
# Also tests TcpApp, which is an Application used to transmit 
# application-level data. Because current TCP isn't capable of this,
# we build this functionality based on byte-stream model of underlying 
# TCP connection.
# 
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-mcache.tcl,v 1.16 2011/10/07 17:43:27 tom_henderson Exp $

#----------------------------------------------------------------------
# Related Files
#----------------------------------------------------------------------
#source misc.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP HttpInval RAP; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
source topologies.tcl

#----------------------------------------------------------------------
# Misc setup
#----------------------------------------------------------------------
set tcl_precision 10
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1


#----------------------------------------------------------------------
# Section 1: Base test class
#----------------------------------------------------------------------
Class Test

# Copied from Simulator::instance{}
Test proc instance {} {
	set t [Test info instances]
	if { $t != "" } {
		return $t
	}
	set tl [Test info subclass] 
	while { $tl != "" } {
		set ntl {}
		foreach t $tl {
			set tt [$t info instances]
			if { $tt != "" } {
				return $tt
			}
			set ntl [eval lappend ntl [$t info subclass]]
		}
		set tl $ntl
	}
	error "Cannot find instance of Test"
}

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
	$self instvar ns_ trace_ net_ defNet_ testName_ node_ test_ topo_ \
			ntrace_

	set ns_ [new Simulator -multicast on]

	set cls [$self info class]
	set cls [split $cls /]
	set test_ [lindex $cls [expr [llength $cls] - 1]]

	global opts
	ns-random $opts(ns-random-seed)

	# XXX We only output LOGs, but no packet traces. 
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

Test instproc create-ranvar { dist args } {
	# options
	array set opts $args
	switch $dist {
		Constant {
			set tmp [new RandomVariable/Constant]
			$tmp set val_ $opts(avg)
		}
		Poisson {
			set tmp [new RandomVariable/Exponential]
			$tmp set avg_ $opts(avg)
		}
		Uniform {
			set tmp [new RandomVariable/Uniform]
			$tmp set min $opts(min)
			$tmp set max $opts(max)
		}
		Pareto {
			set tmp [new RandomVariable/Pareto]
			$tmp set avg_ $opts(avg)
			$tmp set shape_ $opts(shape)
		}
		default {
			error "Unknown random variable distribution $dist"
		}
	}
	if [info exists opts(rng)] {
		$tmp use-rng $opts(rng)
	}
	return $tmp
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
	$self instvar ns_ trace_ ntrace_

	if [info exists trace_] {
		$ns_ flush-trace
		close $trace_
	}
	if [info exists ntrace_] {
		close $ntrace_
	}
	exit 0
}

Test instproc run {} {
	$self instvar finishTime_ ns_ trace_

	$self set-routing
	$self set-members

	$ns_ set-abort-proc "$ns_ flush-trace; \
		$self finish"
	$ns_ at $finishTime_ "$self finish"
	$ns_ run
}

Simulator instproc set-abort-proc { exp } {
	$self instvar abortProc_
	set abortProc_ $exp
}

Simulator instproc abort {} {
	$self instvar abortProc_
	eval $abortProc_
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

# XXX User can reset/append raw_opt_info in their scripts. 
# At the end of user test script, call proc run to start.
# Startup procedure, called at the end of the script
proc run {} {
	global argc argv opts raw_opt_info

	# We don't actually have any real arguments, but we do have 
	# various initializations, which the script depends on.
	process_args

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

# Simple 2 node, one server, one client
Class Topology/2node -superclass SkelTopology

Topology/2node instproc init { ns } {
	$self next 
	$self instvar node_

	set node_(c) [$ns node]
	set node_(s) [$ns node]

	# A modem link + a T1 cross-country link
	$ns duplex-link $node_(c) $node_(s) 10Mb 2ms DropTail
	$ns duplex-link-op $node_(c) $node_(s) orient right
	$ns duplex-link-op $node_(c) $node_(s) queuePos 0.5

	# Possible congestion near the client
	$ns queue-limit $node_(c) $node_(s) 10
}

#
# 3 node linear topology testing SimpleTcp and TcpApp
#
Class Topology/3node -superclass SkelTopology

Topology/3node instproc init { ns } {
	$self next 
	$self instvar node_

	set node_(c) [$ns node]
	set node_(1) [$ns node]
	set node_(s) [$ns node]

	# A modem link + a T1 cross-country link
	$ns duplex-link $node_(c) $node_(1) 56Kb 100ms DropTail
	$ns duplex-link $node_(1) $node_(s) 1.5Mb 50ms DropTail
	$ns duplex-link-op $node_(c) $node_(1) orient right
	$ns duplex-link-op $node_(1) $node_(s) orient right
	$ns duplex-link-op $node_(c) $node_(1) queuePos 0.5
	$ns duplex-link-op $node_(1) $node_(s) queuePos 0.5

	# Possible congestion near the client
	$ns queue-limit $node_(c) $node_(1) 10
	$ns queue-limit $node_(1) $node_(c) 10
}

# Simplest topology: 1 client + 1 cache + 1 server
Class Topology/cache0 -superclass SkelTopology

Topology/cache0 instproc init ns {
	$self next
	$self instvar node_

	set node_(c) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	# A modem link + a T1 cross-country link
	$ns duplex-link $node_(c) $node_(e) 56Kb 100ms DropTail
	$ns duplex-link $node_(e) $node_(s) 1.5Mb 50ms DropTail
	$ns duplex-link-op $node_(c) $node_(e) orient right
	$ns duplex-link-op $node_(e) $node_(s) orient right
	$ns duplex-link-op $node_(c) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(e) $node_(s) queuePos 0.5

	$ns queue-limit $node_(c) $node_(e) 2
	$ns queue-limit $node_(e) $node_(c) 2
	$ns queue-limit $node_(e) $node_(s) 5
	$ns queue-limit $node_(s) $node_(e) 5
}

# Same as cache0 but with the bottleneck link at the server side
Class Topology/cache1 -superclass SkelTopology

Topology/cache1 instproc init ns {
	$self next
	$self instvar node_

	set node_(c) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	# A modem link + a T1 cross-country link
	$ns duplex-link $node_(c) $node_(e) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(e) $node_(s) 56Kb 100ms DropTail
	$ns duplex-link-op $node_(c) $node_(e) orient right
	$ns duplex-link-op $node_(e) $node_(s) orient right
	$ns duplex-link-op $node_(c) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(e) $node_(s) queuePos 0.5

	$ns queue-limit $node_(c) $node_(e) 5
	$ns queue-limit $node_(e) $node_(c) 5
	$ns queue-limit $node_(e) $node_(s) 2
	$ns queue-limit $node_(s) $node_(e) 2
}

#
# 4 nodes: one server, one cache, 2 clients
#
Class Topology/4node -superclass SkelTopology

Topology/4node instproc init ns {
	$self next
	$self instvar node_

	set node_(c0) [$ns node]
	set node_(c1) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	# Ethernet from clients to cache
	$ns duplex-link $node_(c0) $node_(e) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(c1) $node_(e) 1.5Mb 50ms DropTail
	# 56K link from cache to server
	$ns duplex-link $node_(e) $node_(s) 56K 100ms DropTail

	$ns duplex-link-op $node_(c0) $node_(e) orient left
	$ns duplex-link-op $node_(c1) $node_(e) orient down
	$ns duplex-link-op $node_(e) $node_(s) orient left
	$ns duplex-link-op $node_(c0) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(c1) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(e) $node_(s) queuePos 0.5

	$ns queue-limit $node_(c0) $node_(e) 5
	$ns queue-limit $node_(e) $node_(c0) 5
	$ns queue-limit $node_(c1) $node_(e) 5
	$ns queue-limit $node_(e) $node_(c1) 5
	$ns queue-limit $node_(e) $node_(s) 2
	$ns queue-limit $node_(s) $node_(e) 2
}


#
# Heterogeneous 4 nodes: one server, one cache, 2 clients
#
Class Topology/4node-h -superclass SkelTopology

Topology/4node-h instproc init ns {
	$self next
	$self instvar node_

	set node_(c0) [$ns node]
	set node_(c1) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	# Ethernet from clients to cache
	# node c0: abundant bw, node c1: limited bw
	$ns duplex-link $node_(c0) $node_(e) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(c1) $node_(e) 56Kb 50ms DropTail
	# 56K link from cache to server
	$ns duplex-link $node_(e) $node_(s) 56Kb 100ms DropTail

	$ns duplex-link-op $node_(c0) $node_(e) orient left
	$ns duplex-link-op $node_(c1) $node_(e) orient down
	$ns duplex-link-op $node_(e) $node_(s) orient left
	$ns duplex-link-op $node_(c0) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(c1) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(e) $node_(s) queuePos 0.5

	$ns queue-limit $node_(c0) $node_(e) 5
	$ns queue-limit $node_(e) $node_(c0) 5
	$ns queue-limit $node_(c1) $node_(e) 2
	$ns queue-limit $node_(e) $node_(c1) 2
	$ns queue-limit $node_(e) $node_(s) 2
	$ns queue-limit $node_(s) $node_(e) 2
}

# 10 continuous TCP sessions and 10 continuous RAP sessions
Class Topology/mess-h -superclass SkelTopology

Topology/mess-h instproc init ns {
	$self next
	$self instvar node_

	set node_(c0) [$ns node]
	set node_(c1) [$ns node]
	set node_(e) [$ns node]
	set node_(s) [$ns node]

	set d1 [$ns node]
	set d2 [$ns node]

	# Ethernet from clients to cache
	# node c0: abundant bw, node c1: limited bw
	$ns duplex-link $node_(c0) $node_(e) 1.5Mb 50ms DropTail
	$ns duplex-link $node_(c1) $node_(e) 56Kb 50ms DropTail
	# 56K*20 links from cache to server
	$ns duplex-link $node_(e) $d1 1.5Mb 50ms DropTail
	$ns duplex-link $d2 $node_(s) 56Kb 50ms DropTail
	$ns duplex-link $d1 $d2 1.5Mb 100ms DropTail

	$ns duplex-link-op $node_(c0) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(c1) $node_(e) queuePos 0.5
	$ns duplex-link-op $node_(e) $d1 queuePos 0.5
	$ns duplex-link-op $d2 $d1 queuePos 0.5
	$ns duplex-link-op $node_(s) $d2 queuePos 0.5

	# Buffer: 1 RTT at every link, but 4 RTT at the bottleneck link
	$ns queue-limit $node_(c0) $node_(e) 18
	$ns queue-limit $node_(e) $node_(c0) 18
	$ns queue-limit $node_(c1) $node_(e) 3
	$ns queue-limit $node_(e) $node_(c1) 3
	$ns queue-limit $node_(e) $d1 18
	$ns queue-limit $d1 $node_(e) 18
	# 1 RTT for the bottleneck link
	$ns queue-limit $d1 $d2 37
	$ns queue-limit $d2 $d1 37
	# 1 RTT for web server link
	$ns queue-limit $d2 $node_(s) 3
	$ns queue-limit $node_(s) $d2 3

	# Create rap and tcp nodes
	for {set i 0} {$i < 10} {incr i} {
		set node_(t$i) [$ns node]	;# TCP client
		set node_(T$i) [$ns node]	;# TCP server
		set node_(r$i) [$ns node]	;# RAP client
		set node_(R$i) [$ns node]	;# RAP server

		# clients connect to cache, servers connect to dummy
		$ns duplex-link $node_(t$i) $d1 1.5Mb 50ms DropTail
		$ns duplex-link $node_(T$i) $d2 1.5Mb 50ms DropTail
		$ns duplex-link $node_(r$i) $d1 1.5Mb 50ms DropTail
		$ns duplex-link $node_(R$i) $d2 1.5Mb 50ms DropTail
		# Set all queue limits to 1 RTT
		$ns queue-limit $node_(t$i) $d1 18
		$ns queue-limit $d1 $node_(t$i) 18
		$ns queue-limit $node_(T$i) $d2 18
		$ns queue-limit $d2 $node_(T$i) 18
		$ns queue-limit $node_(r$i) $d1 18
		$ns queue-limit $d1 $node_(r$i) 18
		$ns queue-limit $node_(R$i) $d2 18
		$ns queue-limit $d2 $node_(R$i) 18
	}
}


Agent/TCP/FullTcp set segsize_ 1500 	;# segment size 1.5K
Agent/TCP/FullTcp set nodelay_ true	;# don't use Nagle's algorithm
PagePool/Media set page_size_ 1000

# QA-related setup
Application/MediaApp/QA set LAYERBW_ 2500 ;# Byte rate of layer consumption
Application/MediaApp/QA set MAXACTIVELAYERS_ 10
Application/MediaApp/QA set SRTTWEIGHT_ 0.95
Application/MediaApp/QA set SMOOTHFACTOR_ 4
Application/MediaApp/QA set MAXBKOFF_ 100
Application/MediaApp/QA set pref_srtt_ 0.6
Application/MediaApp/QA set debug_output_ 0


#----------------------------------------------------------------------
# Base class for web cache testing
#----------------------------------------------------------------------
Class Test-mcache -superclass Test

Test-mcache set startTime_ 10

Test-mcache instproc init {} {
	$self next
	$self instvar startTime_ log_
	set startTime_ [$class set startTime_]

	# XXX This is the main output of the test suite
	global opts
	if $opts(enable-log) {
		set log_ [open "temp.rands" w]
		$self write-testconf $log_
	}

	# By default set to selective push
	$self set-pagepool

	global opts
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
}

# Allow global options to preempt, and derived classes to overwrite.
Test-mcache instproc set-server-type { servertype } {
	$self instvar serverType_ 
	global opts
	if [info exists opts(server)] {
		set serverType_ $opts(server)
	} else {
		set serverType_ $servertype
	}
}

Test-mcache instproc set-cache-type { cachetype } {
	$self instvar cacheType_ 
	global opts
	if [info exists opts(cache)] {
		set cacheType_ $opts(cache)
	} else {
		set cacheType_ $cachetype
	}
}

Test-mcache instproc set-client-type { clienttype } {
	$self instvar clientType_
	global opts
	if [info exists opts(client)] {
		set clientType_ $opts(client)
	} else {
		set clientType_ $clienttype
	}
}

Test-mcache instproc set-pagepool {} {
	$self instvar startTime_ finishTime_ pgp_
	global opts

	# Use PagePool/Media
	set pgp_ [new PagePool/Media]
	# PagePool/Media MUST know the duration of the simulation; it needs
	# the info to set correct page modification time. 
	$pgp_ set-duration $opts(duration)
	# Number of streams
	$pgp_ set-num-pages $opts(num-pages)
	# Page sizes
	# XXX Should be overwritten later
	for {set i 0} {$i < $opts(num-pages)} {incr i} {
		$pgp_ set-pagesize $i $opts(avg-page-size)
	}
	# Layers
	$pgp_ set-layer $opts(obj-layer)

	# Request generator
	set tmp [new RandomVariable/Uniform]
	$tmp set min_ 0
	$tmp set max_ $opts(num-pages)
	$pgp_ ranvar-req $tmp
	
	$pgp_ set-start-time $startTime_
	set finishTime_ [expr $startTime_ + $opts(duration)]
}

Test-mcache instproc set-req-generator { client } {
	$self instvar pgp_ reqRNG_

	global opts
	if ![info exists reqRNG_] {
		set reqRNG_ [new RNG]
		$reqRNG_ seed $opts(ns-random-seed)
	}
	
	switch $opts(req-dist) {
		Poisson {
			set tmp [new RandomVariable/Exponential]
			$tmp set avg_ $opts(avg-req-interval)
		}
		Pareto {
			set tmp [new RandomVariable/Pareto]
			$tmp set avg_ $opts(avg-req-interval)
			$tmp set shape_ $opts(req-rv-shape)
		}
		Uniform {
			set tmp [new RandomVariable/Uniform]
			set opts(min-req-interval) \
				[expr $opts(avg-req-interval)*0.84]
			set opts(max-req-interval) \
				[expr $opts(avg-req-interval)*1.17]
			$tmp set min_ $opts(min-req-interval)
			$tmp set max_ $opts(max-req-interval)
		}
		default {
			puts "Unkown page request distribution $opts(req-dist)"
			exit 1
		}
	}
	$tmp use-rng $reqRNG_
	$client set-interval-generator $tmp
	$client set-page-generator $pgp_
}

Test-mcache instproc flush-trace {} {
	$self instvar log_
	flush $log_
	[Simulator instance] flush-trace
}

Test-mcache instproc create-members {} {
	$self instvar client_ server_ cache_ log_ test_ pgp_ node_ ns_ \
		serverType_ cacheType_ clientType_

	set st $serverType_
	set lt $clientType_
	# We may not have cache
	if [info exists cacheType_] {
		set ct $cacheType_
	}

	global opts
	foreach n [array names node_] {
		set type [string range $n 0 0]
		set num [string range $n 1 end]
		if {$num == ""} {
			set num 0
		}
		switch $type {
			s {
			 set server_($num) [new Http/Server$st $ns_ $node_($n)]
			 if $opts(enable-log) {
				 $server_($num) log $log_
			 }
			 $server_($num) set-page-generator $pgp_
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

Test-mcache instproc set-routing {} {
	$self instvar ns_ 
	$ns_ rtproto Session
}

Test-mcache instproc set-members {} {
	$self instvar ns_ finishTime_ startTime_
	$ns_ at $startTime_ "$self start-connection"
}

# Create media contents in servers
Test-mcache instproc populate-server {} {
	$self instvar pgp_ cache_ server_ 
	for {set i 0} {$i < [$pgp_ get-poolsize]} {incr i} {
		set pageid $server_(0):$i
		$server_(0) gen-page $pageid
	}
}

Test-mcache instproc start-connection {} {
	$self instvar ns_

	$self create-members
	$self populate-server
	$self set-connections

	# Let initializations settles down, then start requests
	$ns_ at [expr [$ns_ now] + 10] "$self start-requests"
}

Test-mcache instproc finish {} {
	$self instvar log_
	if [info exists log_] {
		close $log_
	}
	$self next
}



# Transmitting a single stream from a client to a server via a bottleneck 
# 56Kb link. NO quality adaptation
Class Test/media1 -superclass Test-mcache

Test/media1 instproc init {} {
	$self set-defnet 3node

	global opts
	set opts(avg-page-size) 102400
	# Since we are NOT doing QA, must use a single layer!!
	set opts(obj-layer) 1

	$self next

	$self set-server-type /Media
	$self set-client-type /Media
	Http set TRANSPORT_ FullTcp
	Http set MEDIA_TRANSPORT_ RAP
}

Test/media1 instproc set-connections {} {
	$self instvar client_ server_ cache_
	$client_(0) connect $server_(0)
}

Test/media1 instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_
	$client_(0) start-session $server_(0) $server_(0)
}



# Simple test of client/cache/server, no quality adaptation
# Bottleneck link (56Kb) is near the client
Class Test/media2 -superclass Test-mcache

Test/media2 instproc init {} {
	$self set-defnet cache0

	global opts
	set opts(avg-page-size) 4096
	set opts(avg-req-interval) 600
	# Since we are NOT doing QA, must use a single layer!!
	set opts(obj-layer) 1

	$self next

	$self set-server-type /Media
	$self set-client-type /Media
	$self set-cache-type /Media
	Http set TRANSPORT_ FullTcp
	Http set MEDIA_TRANSPORT_ RAP
}

Test/media2 instproc set-connections {} {
	$self instvar client_ server_ cache_
	$client_(0) connect $cache_(0)
	$cache_(0) connect $server_(0)
}

Test/media2 instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_
	$client_(0) start-session $cache_(0) $server_(0)
}


# Simple test of QA, one server, one client, 56Kb bottleneck link
Class Test/media3 -superclass Test-mcache

Test/media3 instproc init {} {
	$self set-defnet 3node

	global opts
	set opts(avg-page-size) 524288
	set opts(avg-req-interval) 2000
	set opts(obj-layer) 4
	set opts(duration) 800

	$self next

	$self set-server-type /Media
	$self set-client-type /Media
	Http set TRANSPORT_ FullTcp
	Http set MEDIA_TRANSPORT_ RAP
	Http set MEDIA_APP_ MediaApp/QA
}

Test/media3 instproc set-connections {} {
	$self instvar client_ server_ cache_
	$client_(0) connect $server_(0)
}

Test/media3 instproc start-requests {} {
	$self instvar client_ server_ ns_
	$client_(0) set-cache $server_(0)
	$client_(0) send-request $server_(0) GET $server_(0):0 
}

# Same as above, but 10Mb bottleneck link (basically high enough to hold
# all 8 layers)
Class Test/media3a -superclass Test/media3

Test/media3a instproc init {} {
	$self set-defnet 2node
	$self next
}


# One server, one cache and one client. 
# 56Kb bottleneck link between client and cache.
Class Test/media4 -superclass Test-mcache

Test/media4 instproc init {} {
	$self set-defnet cache0

	global opts
	set opts(avg-req-interval) 60
	set opts(duration) 400
	# Page size set to 300 seconds
	set opts(avg-page-size) 600000
	set opts(obj-layer) 8

	$self next

	$self set-server-type /Media
	$self set-client-type /Media
	$self set-cache-type /Media
	Http set TRANSPORT_ FullTcp
	Http set MEDIA_TRANSPORT_ RAP
	Http set MEDIA_APP_ MediaApp/QA
}

Test/media4 instproc set-connections {} {
	$self instvar client_ server_ cache_
	$client_(0) connect $cache_(0)
	$cache_(0) connect $server_(0)
}

Test/media4 instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_
	$client_(0) set-cache $cache_(0)

	# First request
	$client_(0) send-request $cache_(0) GET $server_(0):0 

	# 5 additional requests
	set time [$ns_ now]
	for {set i 1} {$i < 15} {incr i} {
		incr time 50
		$ns_ at $time "$client_(0) send-request $cache_(0) GET \
$server_(0):0"
	}
}

# Same as media4, but with 56Kb bottleneck link between server and cache.
#Class Test/media4a -superclass Test/media4

#Test/media4a instproc init {} {
#	$self set-defnet cache1
#	$self next
#}


# Test cache replacement
# Clients with heterogeneous bandwidth to the cache: client0-cache is 1.5Mb,
# client1-cache is 56Kb, cache-server is the same as in media5, 56Kb.
#
# We distribute 95% of the requests to the low-bw client 1
Class Test/media5 -superclass Test-mcache

Test/media5 instproc init {} {
	$self set-defnet 4node-h

	global opts
	set opts(num-pages) 3
	set opts(obj-layer) 8
	set opts(cache-sizefac) 0.4
	set opts(total-requests) 10
	set opts(duration) [expr 50*$opts(total-requests)+50]

	$self next

	$self set-server-type /Media
	$self set-client-type /Media
	$self set-cache-type /Media
	Http set TRANSPORT_ FullTcp
	Http set MEDIA_TRANSPORT_ RAP
	Http set MEDIA_APP_ MediaApp/QA
}

Test/media5 instproc set-pagepool {} {
	$self next

	# Set sizes of the pages
	$self instvar pgp_ ns_
	global opts
	set layer $opts(obj-layer)
	set lbw [Application/MediaApp/QA set LAYERBW_]

	# Uniformly distribute stream lengths
	set rv [new RandomVariable/Uniform]
	$rv set min_ 10
	$rv set max_ 20

	$self instvar totalSize_ log_
	set totalSize_ 0
	for {set i 0} {$i < $opts(num-pages)} {incr i} {
		set tmp [expr int([$rv value]*$layer*$lbw)]
		puts $log_ "# Page $i has size $tmp"
		incr totalSize_ $tmp
		$pgp_ set-pagesize $i $tmp
	}
	delete $rv
}

Test/media5 instproc set-connections {} {
	$self instvar client_ server_ cache_ totalSize_

	# Set cache size to be 
	global opts
	set np $opts(num-pages)
	set layer $opts(obj-layer)
	set lbw [Application/MediaApp/QA set LAYERBW_]
	# Set cache size to be 0.5 times total page size
	$cache_(0) set-cachesize [expr $opts(cache-sizefac) * $totalSize_]

	# Establish connection
	$client_(0) connect $cache_(0)
	$client_(1) connect $cache_(0)
	$cache_(0) connect $server_(0)
}

Test/media5 instproc start-requests {} {
	$self instvar client_ cache_ server_ ns_ log_

	# Client setting parent caches
	$client_(0) set-cache $cache_(0)
	$client_(1) set-cache $cache_(0)

	# Generate non-overlapping request sequence
	global opts
	set np $opts(num-pages)
	set time [$ns_ now]
	set tr $opts(total-requests)

	# Build page popularity table according to total requests and 
	# Zipf's law
	set omega 0
	for {set i 1} {$i <= $np} {incr i} {
		set omega [expr $omega + 1.0/$i]
	}
	set omega [expr 1.0 / $omega]
	# Calculate number of requests for each page
	set j 0
	for {set i 0} {$i < $np} {incr i} {
		set nreq($i) [expr round($omega*$tr/($i+1.0))]
		for {set ii 0} {$ii < $nreq($i)} {incr ii} {
			set tmp1($j) 0	;# Whether this request is occupied
			set tmp2($j) $i ;# which page this request belong to
			incr j
		}
		puts $log_ "# Total $nreq($i) requests for page $i"
	}
	if {$j != $tr} {
		error "Mis-calculated number of requests: this $j orig $tr"
	}
	# Build request sequence, uniform distribution
	set rv [new RandomVariable/Uniform]
	$rv set min_ 0
	$rv set max_ $j
	set i 0
	set time 0
	$self instvar reqlist_
	# Schedule requests, 500s interval should suffice for 56K bottleneck
	# link and 600s stream with 8 layers of 2.5K bw.
	while {$i < $j} {
		set rid [expr int([$rv value])]
		if {$tmp1($rid) != 0} {
			# Already allocated
			continue
		}
		set tmp1($rid) 1
		# Determine client ID
		set cid [$self req2client $tmp2($rid) [$rv value] $j]

		# Instead of scheduling a request right now, let's do it
		# step by step so that scheduler won't complain.
		lappend reqlist_ $cid $tmp2($rid)

		# XXX Interval 500 second. Should adjust it according to 
		# maximum page size
		incr time 50
		incr i
	}
	$self next-request
}

# Get the first request from reqlist_
Test/media5 instproc next-request {} {
	$self instvar server_ cache_ client_ reqlist_ ns_
	set cid [lindex $reqlist_ 0]
	set pagenum [lindex $reqlist_ 1]
	set reqlist_ [lrange $reqlist_ 2 end]

	$client_($cid) send-request $cache_(0) GET $server_(0):$pagenum

	# Do our next request 50 seconds later
	if {[llength $reqlist_] > 0} {
		# If we still have more requests, continue
		$ns_ at [expr [$ns_ now] + 50] "$self next-request"
	}
}

# Use a random number and the upper bound of the random number to 
# decide whether the request goes to client 0 or 1
Test/media5 instproc req2client { pagenum ran max } {
	# Most requests go to the low-bw client
	set thresh [expr $max * 0.95]
	if {$ran > $thresh} {
		return 0
	} else {
		return 1
	}
}

# 95% requests go to the high-bw client
#Class Test/media5a -superclass Test/media5 

#Test/media5a instproc req2client { pagenum ran max } {
#	# Distribute 95% requests of all pages to the high-bw client, 
#	# and 5% to the low-bw client
#	set thresh [expr $max*0.05]
#	if {$ran > $thresh} {
#		return 0
#	} else {
#		return 1
#	}
#}

# 50:50 distribution among 2 clients
#Class Test/media5b -superclass Test/media5
#
#Test/media5b instproc req2client { pagenum ran max } {
#	# Uniformly distribute requests of all pages
#	set thresh [expr $max*0.5]
#	if {$ran > $thresh} {
#		return 0
#	} else {
#		return 1
#	}
#}


# Configurations
global raw_opt_info
set raw_opt_info {
	# Random number seed; default is 0, so ns will give a 
	# diff. one on each invocation.
	# XXX Get a "good" seed from predef_seeds[] in rng.cc
	ns-random-seed 188312339
		
	# Animation options; complete traces are useful
	# for nam only, so do those only when a tracefile
	# is being used for nam
	enable-log 1
	duration 500

	# Packet size configurations
	cache-ims-size	50
	cache-ref-size	50
	server-inv-size	43
	client-req-size 43

	# request intervals
	avg-req-interval 1000
	req-dist Poisson

	# Cache size factor, i.e., what % of the total stream size
	cache-sizefac 0.5
	# Object layers
	obj-layer 8
	# One media stream takes 5 layers, and 512K in total
	avg-page-size 512000
	# Number of pages 
	num-pages 1
	# Total number of requests
	total-requests 10
}


#----------------------------------------------------------------------
# Execution starts...
#----------------------------------------------------------------------
run
