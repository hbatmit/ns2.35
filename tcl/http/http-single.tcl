#
# Copyright (c) 1997 Regents of the University of California.
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
# 
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
# This file contributed by Curtis Villamizar <curtis@ans.net>, May 1997.
# Maintainer: John Heidemann <johnh@isi.edu>.
#

#
# WARNING:  This code uses the compatibility library and so should not
# be used as an example.  Hopefully at some time in the future it will
# be updated.
#

# Create a three node test environment
#
#  c1 -------- isp1 -------- s1
#      28.8kb        1.54mb
#     modemdelay    netdelay
#

proc create_testnet { } {
    global testnet

    set testnet(c1) [ns node]
    set testnet(isp1) [ns node]
    set testnet(s1) [ns node]

    set testnet(L1) \
	    [ns_duplex $testnet(c1) $testnet(isp1) \
	    $testnet(modemspeed) $testnet(modemdelay) $testnet(qtype)]
    ns_duplex $testnet(isp1) $testnet(s1) \
	    $testnet(netspeed) $testnet(netdelay) drop-tail
    [lindex $testnet(L1) 0] set queue-limit $testnet(modemqueue)
    [lindex $testnet(L1) 1] set queue-limit $testnet(modemqueue)
    if {$testnet(qtype) == "red"} {
	set redlink [ns link $testnet(c1) $testnet(isp1)]
	$redlink set thresh [expr $testnet(modemqueue) * 0.25]
	$redlink set maxthresh [expr $testnet(modemqueue) * 0.85]
	$redlink set q_weight 0.001
	$redlink set wait_ 1
	$redlink set dropTail_ 1
    }
}

proc tcpDump { tcpSrc interval } {
    proc dump { src interval } {
	ns at [expr [ns now] + $interval] "dump $src $interval"
	puts [ns now]/ack=[$src get ack]
    }
    ns at 0.0 "dump $tcpSrc $interval"
}

proc trigger { xresults } {
    global testnet flows

    # NEEDSWORK:  should we really indirect once down results like this?
    set results "[lindex $xresults 0]"
    set type [lindex $results 0]
    if { $type != "-" && $type != "d" } {
	return;
    }
    set id [lindex $results 7]
    set counter [format "count%d" $id]
    if { [info exists flows($counter)] } {
	incr flows($counter)
    } else {
	set flows($counter) 1
    }
    if { $type != "-" } {
	return;
    }
    set got [expr 1 + [lindex $results 10]]
    set isrunning isrunning$id
    if { $id == 0 } {
	if { $got == $testnet(httpsize) } {
	    puts [format "http flow completed at %s" [lindex $results 1]]
	    incr flows(flows_running) -1
	    set flows($isrunning) 0
	}
    } else {
	if { $got == $testnet(inlinesize) } {
	    puts [format "inline%d completed at %s" $id [lindex $results 1]]
	    incr flows(flows_running) -1
	    incr flows(inlines_running) -1
	    set flows($isrunning) 0
	}
    }
    set flows(persist) 1
    if { $flows(count0) >= 1 } {
	while {$flows(inlines_started) < $flows(inlines_needed) \
		&& $flows(flows_running) < $flows(flows_allowed) \
		&& $flows(inlines_running) < $flows(inlines_allowed) } {
	    incr flows(inlines_started)
	    incr flows(flows_running)
	    incr flows(inlines_running)
	    set ident $flows(inlines_started)
	    set nextflow inline$ident
	    if {$flows(persist)} {
		for {set j 0} {$j < $ident} {incr j} {
		    set isrunning isrunning$j
		    if {$flows($isrunning) == 0} {
			set thisflow tcp$ident
			if {$j == 0} {
			    set otherflow tcp0
			} else {
			    set otherflow tcp$j
			}
			$flows($thisflow) persist $flows($otherflow)
			break
		    }
		}
	    }
	    $flows($nextflow) start
	    set isrunning isrunning$ident
	    set flows($isrunning) 1
	    puts [format "trigger at %s: start %d" \
		    [lindex $results 1] $flows(inlines_started)]
	}
    }
}

proc openTrace { stopTime testName } {
    exec rm -f out.tr temp.rands
    set traceFile [open out.tr w]
    ns at $stopTime "close $traceFile ; finish $testName"
    set T [ns trace]
    $T attach $traceFile
    return $T
}

proc finish file {
    global testnet flows

    set f [open temp.rands w]
    puts $f "TitleText: $file"
    puts $f "Device: Postscript"
    
    set total 0
    for { set i 0 } { $i <= $flows(inlines_needed) } { incr i } {
	set counter [format "count%d" $i]
	set got $flows($counter)
	incr total $got
	puts [format "flow %d : %d packets" $i $got]
    }
    set needed [expr $testnet(httpsize) \
	    + ( $flows(inlines_needed) * $testnet(inlinesize) )]
    set discard [expr $total - $needed]
    puts [format "%d sent : %d needed : %d discarded : %d %%" \
	    $total $needed $discard [expr 100 * $discard / $needed]]
    if { $testnet(dograph) == 0 } {
	exit 0
    }
    exec rm -f temp.p temp.d 
    exec touch temp.d temp.p
    #
    # split queue/drop events into two separate files.
    # we don't bother checking for the link we're interested in
    # since we know only such events are in our trace file
    #
    exec awk {
	{
	    if (($1 == "-" ) && \
		    ($5 == "tcp" || $5 == "ack") && \
		    ($8 == 0 || ($8 == 4 && $11 <= 6))) \
		    print $2, $8 + ($11 % 90) * 0.01
	}
    } out.tr > temp.p1
    exec awk {
	{
	    if (($1 == "-" ) && \
		    ($5 == "tcp" || $5 == "ack") && \
		    ($8 == 1 || ($8 == 4 && $11 > 6 && $11 <= 26))) \
		    print $2, $8 + ($11 % 90) * 0.01
	}
    } out.tr > temp.p2
    exec awk {
	{
	    if (($1 == "-" ) && \
		    ($5 == "tcp" || $5 == "ack") && \
		    ($8 == 2 || $8 == 3 || ($8 == 4 && $11 > 26))) \
		    print $2, $8 + ($11 % 90) * 0.01
	}
    } out.tr > temp.p3
    exec awk {
	{
	    if ($1 == "d")
	    print $2, $8 + ($11 % 90) * 0.01
	}
    } out.tr > temp.d

    puts $f \"packets
    flush $f
    exec cat temp.p1 >@ $f
    flush $f
    puts $f [format "\n\"1st-inline\n"]
    flush $f
    exec cat temp.p2 >@ $f
    flush $f
    puts $f [format "\n\"other-2\n"]
    flush $f
    exec cat temp.p3 >@ $f
    flush $f
    # insert dummy data sets so we get X's for marks in data-set 4
    # puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]
    
    puts $f [format "\n\"drops\n"]
    flush $f
    #
    # Repeat the first line twice in the drops file because
    # often we have only one drop and xgraph won't print marks
    # for data sets with only one point.
    #
    exec head -1 temp.d >@ $f
    exec cat temp.d >@ $f
    close $f
    exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
    
    exit 0
}

proc init_tcp_flow {taskid flowid id size} {
    global testnet flows

    set flow [ns_create_connection tcp-reno \
	    $testnet(s1) tcp-sink $testnet(c1) $id]
    $flow set window $testnet(window)
    $flow set packet-size $testnet(mss)
    $flow set maxcwnd $testnet(window)
    set flows($flowid) $flow 
    set flows($taskid) [$flow source ftp]
    $flows($taskid) set maxpkts_ $size
    # tcpDump $flow $testnet(dumpincr)
}

proc setup_http_test {} {
    global testnet
    global flows

    create_testnet

    init_tcp_flow http tcp0 0 $testnet(httpsize)
    ns at 0.0 "$flows(http) start"
    set flows(isrunning0) 1
    set flows(flows_running) 1
    set flows(count0) 0

    for { set i 1 } { $i <= $flows(inlines_needed) } { incr i } {
	set nexttask [format "inline%d" $i]
	set nextflow [format "tcp%d" $i]
	init_tcp_flow $nexttask $nextflow $i $testnet(inlinesize)
    }

    # trace only the bottleneck link
    set traceme [openTrace $testnet(testlimit) test_http]
    set bottleneck [ns link $testnet(isp1) $testnet(c1)]
    $bottleneck trace $traceme
    $bottleneck callback { trigger }
}

proc set_globals {} {
    global testnet
    set testnet(netspeed) 1.54mb
    set testnet(modemspeed) 28.8kb
    set testnet(netdelay) 150ms
    set testnet(modemdelay) 50ms
    set testnet(mss) 512
    set testnet(window) 64
    set testnet(httpsize) 6
    set testnet(inlinesize) 40
    set testnet(modemqueue) 6
    set testnet(dumpincr) 5.0
    set testnet(testlimit) 50.0
    set testnet(dograph) 0
    set testnet(qtype) drop-tail

    global flows
    set flows(inlines_started) 0
    set flows(inlines_needed) 3
    set flows(flows_running) 0
    set flows(flows_allowed) 4
    set flows(inlines_running) 0
    set flows(inlines_allowed) $flows(flows_allowed)
}

proc process_args {} {
    global argc argv testnet flows

    for {set i 0} {$i < $argc} {incr i} {
	set arg [lindex $argv $i]
	switch x$arg {
	    x-window {
		incr i
		set testnet(window) [lindex $argv $i]
	    }
	    x-graph {
		set testnet(dograph) 1
		puts "match"
	    }
	    x-delayN {
		incr i
		set testnet(netdelay) [lindex $argv $i]
	    }
	    x-delayM {
		incr i
		set testnet(modemdelay) [lindex $argv $i]
	    }
	    x-mss {
		incr i
		set testnet(mss) [lindex $argv $i]
	    }
	    x-httpsize {
		incr i
		set testnet(httpsize) [lindex $argv $i]
	    }
	    x-inlinesize {
		incr i
		set testnet(inlinesize) [lindex $argv $i]
	    }
	    x-queue {
		incr i
		set testnet(modemqueue) [lindex $argv $i]
	    }
	    x-dumpincr {
		incr i
		set testnet(dumpincr) [lindex $argv $i]
	    }
	    x-testlimit {
		incr i
		set testnet(testlimit) [lindex $argv $i]
	    }
	    x-inlines {
		incr i
		set flows(inlines_needed) [lindex $argv $i]
	    }
	    x-maxflow {
		incr i
		set flows(flows_allowed) [lindex $argv $i]
	    }
	    x-maxinline {
		incr i
		set flows(inlines_allowed) [lindex $argv $i]
	    }
	    x-red {
		set testnet(qtype) red
	    }
	    x-sfq {
		set testnet(qtype) sfq
	    }
	    default {
		puts [format "unrecognized argument: %s" [lindex $argv $i]]
		exit 1
	    }
	}
    }
}

set_globals
process_args
setup_http_test
# ns gen-map
ns run
