#
# Copyright (c) 1995 The Regents of the University of California.
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
#	This product includes software developed by the Computer Systems
#	Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/test-suite.tcl,v 1.2 2004/08/17 15:26:51 johnh Exp $

#
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., Simulator Tests. July 1995.  
# URL ftp://ftp.ee.lbl.gov/papers/simtests.ps.Z.
#
# To run all tests:  test-all
# To run individual tests:
# ns test-suite.tcl tahoe1
# ns test-suite.tcl tahoe2
# ...
#
# Create a simple four node topology:
#
#        s1
#         \ 
#  8Mb,5ms \  0.8Mb,100ms
#           r1 --------- k1
#  8Mb,5ms /
#         /
#        s2
#
proc create_testnet { } {

	global s1 s2 r1 k1
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set k1 [ns node]

	ns_duplex $s1 $r1 8Mb 5ms drop-tail
	ns_duplex $s2 $r1 8Mb 5ms drop-tail
	set L [ns_duplex $r1 $k1 800Kb 100ms drop-tail]
	[lindex $L 0] set queue-limit 6
	[lindex $L 1] set queue-limit 6
}

proc create_testnet1 { } {

	global s1 s2 r1 k1
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set k1 [ns node]

	ns_duplex $s1 $r1 10Mb 5ms drop-tail
	ns_duplex $s2 $r1 10Mb 5ms drop-tail
	set L [ns_duplex $r1 $k1 1.5Mb 100ms drop-tail]
	[lindex $L 0] set queue-limit 23
	[lindex $L 1] set queue-limit 23
}

#
# Create a simple six node topology:
#
#        s1		    s3
#         \ 		    /
# 10Mb,2ms \  1.5Mb,20ms   / 10Mb,4ms
#           r1 --------- r2
# 10Mb,3ms /		   \ 10Mb,5ms
#         /		    \
#        s2		    s4 
#
proc create_testnet2 { } {

	global s1 s2 r1 r2 s3 s4
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set r2 [ns node]
	set s3 [ns node]
	set s4 [ns node]

	ns_duplex $s1 $r1 10Mb 2ms drop-tail
	ns_duplex $s2 $r1 10Mb 3ms drop-tail
	set L [ns_duplex $r1 $r2 1.5Mb 20ms red]
	[lindex $L 0] set queue-limit 25
	[lindex $L 1] set queue-limit 25
	ns_duplex $s3 $r2 10Mb 4ms drop-tail
	ns_duplex $s4 $r2 10Mb 5ms drop-tail
}

proc finish file {

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p temp.d 
	exec touch temp.d temp.p
	#
	# split queue/drop events into two separate files.
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	exec awk {
		{
			if (($1 == "+" || $1 == "-" ) && \
			    ($5 == "tcp" || $5 == "ack"))\
					print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr > temp.p
	exec awk {
		{
			if ($1 == "d")
				print $2, $8 + ($11 % 90) * 0.01
		}
	} out.tr > temp.d

	puts $f \"packets
	flush $f
	exec cat temp.p >@ $f
	flush $f
	# insert dummy data sets so we get X's for marks in data-set 4
	puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]

	puts $f \"drops
	flush $f
	#
	# Repeat the first line twice in the drops file because
	# often we have only one drop and xgraph won't print marks
	# for data sets with only one point.
	#
	exec head -n 1 temp.d >@ $f
	exec cat temp.d >@ $f
	close $f
	exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
	
	exit 0
}

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
proc tcpDump { tcpSrc interval } {
	proc dump { src interval } {
		ns at [expr [ns now] + $interval] "dump $src $interval"
		puts [ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]
	}
	ns at 0.0 "dump $tcpSrc $interval"
}

proc tcpDumpAll { tcpSrc interval label } {
	proc dump { src interval label } {
		ns at [expr [ns now] + $interval] "dump $src $interval $label"
		puts $label/time=[ns now]/cwnd=[$src get cwnd]/ssthresh=[$src get ssthresh]/ack=[$src get ack]/rtt=[$src get rtt]
	}
	puts $label:window=[$tcpSrc get window]/packet-size=[$tcpSrc get packet-size]/bug-fix=[$tcpSrc get bug-fix]
	ns at 0.0 "dump $tcpSrc $interval $label"
}

proc openTrace { stopTime testName } {
	exec rm -f out.tr temp.rands
	global r1 k1    
	set traceFile [open out.tr w]
	ns at $stopTime \
		"close $traceFile ; finish $testName"
	set T [ns trace]
	$T attach $traceFile
	return $T
}

proc test_tahoe1 {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 50
	set ftp1 [$tcp1 source ftp]
	ns at 0.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 5.0 test_tahoe]

	ns run
}

proc test_tahoe2 {} {
	global s1 s2 r1 k1
	create_testnet

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 14
	set ftp1 [$tcp1 source ftp]

	ns at 1.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 5.0 test_tahoe2]

	ns run
}

proc test_tahoe3 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 8
	[ns link $k1 $r1] set queue-limit 8
	
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 100

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 16

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 0.5 "$ftp2 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 8.0 test_tahoe3]

	ns run
}

proc test_tahoe4 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 200ms
	[ns link $r1 $s2] set delay 200ms
	[ns link $r1 $k1] set queue-limit 11
	[ns link $k1 $r1] set queue-limit 11
	
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 30

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 30

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 0.0 "$ftp1 start"
	ns at 0.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 25.0 test_tahoe4]

	ns run
}

proc test_no_bug {} {

	global s1 s2 r1 k1
	create_testnet1
	[ns link $s1 $r1] set delay 3ms
	[ns link $r1 $s1] set delay 3ms 

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 50

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 50

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.75 "$ftp2 produce 100"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 6.0 test_no_bug]

	ns run
}

proc test_bug {} {

	global s1 s2 r1 k1
	create_testnet1
	[ns link $s1 $r1] set delay 3ms
	[ns link $r1 $s1] set delay 3ms 

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 50
	$tcp1 set bug-fix false

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 50
	$tcp2 set bug-fix false

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.75 "$ftp2 produce 100"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 6.0 test_bug]

	ns run
}

proc test_reno1 {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	$tcp1 set window 14

	set ftp1 [$tcp1 source ftp]
	ns at 1.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 5.0 test_reno1]

	ns run
}

proc test_reno {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	$tcp1 set maxcwnd 14

	set ftp1 [$tcp1 source ftp]
	ns at 1.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 5.0 test_reno]

	ns run
}

proc test_renoA {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 8
	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 4
	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 4

	set ftp1 [$tcp1 source ftp]
	ns at 1.0 "$ftp1 start"
	set ftp2 [$tcp2 source ftp]
	ns at 1.2 "$ftp2 start"
	$ftp2 set maxpkts 7
	set ftp3 [$tcp3 source ftp]
	ns at 1.2 "$ftp3 start"
	$ftp3 set maxpkts 7

	tcpDump $tcp1 1.0
	tcpDump $tcp2 1.0
	tcpDump $tcp3 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 5.0 test_renoA]

	ns run
}

proc test_reno2 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 9

	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	$tcp1 set window 50

	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $k1 1]
	$tcp2 set window 20

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 10.0 test_reno2]

	ns run
}

proc test_reno3 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 8
	[ns link $k1 $r1] set queue-limit 8

	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	$tcp1 set window 100

	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $k1 1]
	$tcp2 set window 16

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 0.5 "$ftp2 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 8.0 test_reno3]

	ns run
}

proc test_reno4 {} { 
        global s1 s2 r1 r2 s3 s4
        create_testnet2
        [ns link $r1 $r2] set queue-limit 29
        set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink-da $r2 0]
        $tcp1 set window 80
 	$tcp1 set maxcwnd 40

        set ftp1 [$tcp1 source ftp]
        ns at 0.0 "$ftp1 start"

        tcpDump $tcp1 1.0

        # trace only the bottleneck link
        [ns link $s1 $r1] trace [openTrace 2.0 test_reno4]

        ns run
}

proc test_reno4a {} { 
        global s1 s2 r1 r2 s3 s4
        create_testnet2
        [ns link $r1 $r2] set queue-limit 29
        set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink-da $r2 0]
        $tcp1 set window 40
	$tcp1 set maxcwnd 40

        set ftp1 [$tcp1 source ftp]
        ns at 0.0 "$ftp1 start"

        tcpDump $tcp1 1.0

        # trace only the bottleneck link
        [ns link $s1 $r1] trace [openTrace 2.0 test_reno4]

        ns run
}

proc test_reno5 {} {
        global s1 s2 r1 k1
        create_testnet
        [ns link $r1 $k1] set queue-limit 9

        set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
        $tcp1 set window 50
        $tcp1 set bug-fix false

        set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $k1 1]
        $tcp2 set window 20
        $tcp2 set bug-fix false

        set ftp1 [$tcp1 source ftp]
        set ftp2 [$tcp2 source ftp]

        ns at 1.0 "$ftp1 start"
        ns at 1.0 "$ftp2 start"

        tcpDump $tcp1 1.0

        # trace only the bottleneck link
        [ns link $r1 $k1] trace [openTrace 10.0 test_reno5]

        ns run
}

proc test_telnet {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 8
	[ns link $k1 $r1] set queue-limit 8

	set tcp1 [ns_create_connection tcp-reno $s1 tcp-sink $k1 0]
	set tcp2 [ns_create_connection tcp-reno $s2 tcp-sink $k1 1]
	set tcp3 [ns_create_connection tcp-reno $s2 tcp-sink $k1 2]

	set telnet1 [$tcp1 source telnet] ; $telnet1 set interval 1
	set telnet2 [$tcp2 source telnet] ; $telnet2 set interval 0
 	# Interval 0 designates the tcplib telnet interarrival distribution
	set telnet3 [$tcp3 source telnet] ; $telnet3 set interval 0

	ns at 0.0 "$telnet1 start"
	ns at 0.0 "$telnet2 start"
	ns at 0.0 "$telnet3 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 50.0 test_telnet]

	# use a different seed each time
	puts seed=[ns random 0]

	ns run
}

proc test_delayed {} {
	global s1 s2 r1 k1
	create_testnet
	set tcp1 [ns_create_connection tcp $s1 tcp-sink-da $k1 0]
	$tcp1 set window 50

	# lookup up the sink and set it's delay interval
	[$k1 agent [$tcp1 dst-port]] set interval 100ms

	set ftp1 [$tcp1 source ftp]
	ns at 1.0 "$ftp1 start"

	tcpDump $tcp1 1.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 4.0 test_delayed]

	ns run
}

proc test_phase {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 3ms
	[ns link $r1 $s2] set delay 3ms
	[ns link $r1 $k1] set queue-limit 16
	[ns link $k1 $r1] set queue-limit 100

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 32

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 32

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 5.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 25.0 test_phase]

	ns run
}

proc test_phase1 {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 9.5ms
	[ns link $r1 $s2] set delay 9.5ms
	[ns link $r1 $k1] set queue-limit 16
	[ns link $k1 $r1] set queue-limit 100

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 32

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 32

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 5.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 25.0 test_phase1]

	ns run
}

proc test_phase2 {} {
        global s1 s2 r1 k1
        create_testnet  
        [ns link $s2 $r1] set delay 3ms
        [ns link $r1 $s2] set delay 3ms
        [ns link $r1 $k1] set queue-limit 16
        [ns link $k1 $r1] set queue-limit 100

        set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
        $tcp1 set window 32
        $tcp1 set overhead 0.01

        set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
        $tcp2 set window 32
        $tcp2 set overhead 0.01
        
        set ftp1 [$tcp1 source ftp]
        set ftp2 [$tcp2 source ftp]
 
        ns at 5.0 "$ftp1 start"
        ns at 1.0 "$ftp2 start"
        
        tcpDump $tcp1 5.0
 
        # trace only the bottleneck link
        [ns link $r1 $k1] trace [openTrace 25.0 test_phase2]

        ns run
}

proc test_timers {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $r1 $k1] set queue-limit 2
	[ns link $k1 $r1] set queue-limit 100

	set tcp1 [ns_create_connection tcp $s1 tcp-sink-da $k1 0]
	$tcp1 set window 4
	# lookup up the sink and set it's delay interval
	[$k1 agent [$tcp1 dst-port]] set interval 100ms

	set tcp2 [ns_create_connection tcp $s2 tcp-sink-da $k1 1]
	$tcp2 set window 4
	# lookup up the sink and set it's delay interval
	[$k1 agent [$tcp2 dst-port]] set interval 100ms

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.3225 "$ftp2 start"

	tcpDump $tcp1 5.0

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 10.0 test_timers]

	ns run
}

proc test_stats {} {
	global s1 s2 r1 k1
	create_testnet
	[ns link $s2 $r1] set delay 200ms
	[ns link $r1 $s2] set delay 200ms
	[ns link $r1 $k1] set queue-limit 10
	[ns link $k1 $r1] set queue-limit 10
	
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 30

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 30

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	tcpDumpAll $tcp1 5.0 tcp1
	tcpDumpAll $tcp2 5.0 tcp2

	# trace only the bottleneck link
	[ns link $r1 $k1] trace [openTrace 10.0 test_stats]

	ns run
}

if { $argc != 1 } {
	puts stderr {usage: ns test-suite.tcl [ tahoe1 tahoe2 ... reno reno2 ... ]}
	exit 1
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "test-suite.tcl: no such test: $argv"
}
test_$argv

