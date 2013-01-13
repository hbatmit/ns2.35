
#
# callback_demo.tcl
# $Id: callback_demo.tcl,v 1.3 1998/09/02 20:38:42 tomh Exp $
#
# Copyright (c) 1997 University of Southern California.
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Author/maintainer: John Heidemann <johnh@isi.edu>
#

proc usage {} {
	puts stderr {usage: ns callback_demo.tcl [options]

This program exists to demonstrate tracing via callback procedures
rather than files.

Compare
	ns callback_demo.tcl -trace-callback none
which creates the file callback_demo.tr

with
	ns callback_demo.tcl -trace-callback print_traces
and
	ns callback_demo.tcl -trace-callback print_dequeue_traces
which invokes a callback to print traces to stdout.

Look at the functions print_traces and print_dequeue_traces
for examples of how to implement your
own callbacks.
}
	exit 1
}

Class TestFeature

Application/FTP instproc fire {} {
	global opts
	$self instvar maxpkts_
	set maxpkts_ [expr $maxpkts_ + $opts(web-page-size)]
	$self produce $maxpkts_
}


TestFeature instproc print_traces {args} {
	# if you want args not as a list, call the parameter something else
	# see proc(n) for why.
	puts "print_traces: $args"
}

#
# This function filters out everything but dequeue events.
# A better way to do this might be to only attach the trace
# to the deqT_ trace event, but that requires that you do
# something like SimpleLink::trace-callback.
#
TestFeature instproc print_dequeue_traces {a} {
	# don't call the param a so that lindex works without
	# another level of indirection.
	set event_type [lindex $a 0]
	if {$event_type == "-"} {
		puts "print_dequeue_traces $a"
	} else {
		# ignore the trace
	}
}

TestFeature instproc init {} {
	global opts

	# network
	$self instvar ns_ node1_ node2_ link12_
	set ns_ [new Simulator]
	set node1_ [$ns_ node]
	set node2_ [$ns_ node]
	$ns_ duplex-link $node1_ $node2_ 8Mb 100ms DropTail
	# this is gross!
 	set link12_ [$ns_ link $node1_ $node2_]

	# traffic
	$self instvar tcp_ ftp_
	set tcp_ [$ns_ create-connection TCP/Reno $node1_ TCPSink/DelAck $node2_ 0]
	set ftp_ [$tcp_ attach-app FTP]
	$ftp_ set maxpkts_ 0
	$ns_ at 0 "$ftp_ fire"

	# traces

	if {$opts(trace-callback) != "none"} {
		$link12_ trace-callback $ns_ "$self $opts(trace-callback)"
	} else {
		$self instvar trace_file_
		set trace_file_ [open $opts(output) w]
		$link12_ trace $ns_ $trace_file_
	}

	# run things
	$ns_ at $opts(duration) "$self finish"
	$ns_ run
}


TestFeature instproc finish {} {
	$self instvar trace_file_
	if [info exists trace_file_] {
		close $trace_file_
	}

	exit 0
}


proc default_options {} {
	global opts opt_wants_arg

	set raw_opt_info {
		duration 10
		output callback_demo.tr

		# packet size is 1000B
		# web page size in 10 pkts
		web-page-size 10

		# boolean:
		trace-callback none
	}

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
		} else {
			set opt_wants_arg($key) 0
			# die "unknown stuff in raw_opt_info\n"
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
		regsub {^-} $key {} key
		if {![info exists opt_wants_arg($key)]} {
			puts stderr "unknown option $key";
			usage
		}
		if {$opt_wants_arg($key)} {
			incr i
			set opts($key) [lindex $argv $i]
		} else {
			set opts($key) [expr !opts($key)]
		}
	}
}

proc main {} {
	process_args
	new TestFeature
}

main

	
