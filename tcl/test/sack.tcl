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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/sack.tcl,v 1.2 2005/05/29 15:35:19 sfloyd Exp $
#

#
# This file uses the ns-1 interfaces, not the newere v2 interfaces.
# Don't copy this code for use in new simulations,
# copy the new code with the new interfaces!
#

Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP/Newreno set newreno_changes1_ 0
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.
Agent/TCP/Newreno set partial_window_deflation_ 0
# The default is being changed to 1 on 5/5/03, to reflect RFC 2582.
Agent/TCP set singledup_ 0
# The default has been changed to 1

#
# Create a simple four node topology:
#
#        s1
#         \ 
#  8Mb,0ms \  0.8Mb,100ms
#           r1 --------- k1
#  8Mb,0ms /
#         /
#        s2
#
# one drop
proc create_testnet { } {

	global mod s1 s2 r1 r2 k1
	global mod ns_tcp ns_tcpnewreno
	set mod 100
	set s1 [ns node]
	set s2 [ns node]
	set r1 [ns node]
	set k1 [ns node]
	set r2 [ns node]

	ns_duplex $s1 $r1 8Mb 0.1ms drop-tail
	ns_duplex $s2 $r1 8Mb 0.1ms drop-tail
	ns_duplex $r1 $r2 8Mb 0.001ms drop-tail
	set L [ns_duplex $r2 $k1 800Kb 100ms drop-tail]
	[lindex $L 0] set queue-limit 6
	[lindex $L 1] set queue-limit 6
	# maxburst must be set to an integer.

	set ns_tcp(maxburst) maxBurst
	set ns_tcp(bug-fix) false
	set ns_tcpnewreno(changes) 0
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

	global mod s1 s2 r1 r2 r2 s3 s4
	global mod ns_tcp
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
	set ns_tcp(maxburst) maxBurst
	set ns_tcp(bug-fix) false
}

proc finish { file mod } {

	set f [open temp.rands w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"
	
	exec rm -f temp.p 
	exec touch temp.p
	#
	# split queue/drop events into two separate files.
	# we don't bother checking for the link we're interested in
	# since we know only such events are in our trace file
	#
	exec awk {
		{
			if (($1 == "+" || $1 == "-" ) && \
			    ($5 == "tcp" || $5 == "ack"))\
					print $2, $8*(mod+10) + ($11 % mod) 
		}
	} mod=$mod out.tr > temp.p

	exec rm -f temp.d
	exec touch temp.d
	exec awk {
		{
			if ($1 == "d")
				print $2, $8*(mod+10) + ($11 % mod) 
		}
	} mod=$mod out.tr > temp.d

        exec rm -f temp.p1
	exec touch temp.p1
        exec awk {
                {
                        if (($1 == "-" ) && \
                            ($5 == "tcp" || $5 == "ack"))\
                                        print $2, $8*(mod+10) + ($11 % mod) 
                }
        } mod=$mod out.tr1 > temp.p1

	puts $f \"packets
	flush $f
	exec cat temp.p >@ $f
	flush $f
        puts $f \n\"acks
        flush $f
        exec cat temp.p1 >@ $f
	# insert dummy data sets so we get X's for marks in data-set 4
#	puts $f [format "\n\"skip-1\n0 1\n\n\"skip-2\n0 1\n\n"]
	puts $f [format "\n\"skip-1\n0 1\n\n"]

	puts $f \"drops
	flush $f
	#
	# Repeat the first line twice in the drops file because
	# often we have only one drop and xgraph won't print marks
	# for data sets with only one point.
	#
	exec head -1 temp.d >@ $f
	exec cat temp.d >@ $f
	close $f
#	exec csh figure.com $file &
	puts stdout "Calling xgraph."
	exec xgraph -bb -tk -nl -m -ly 0,$mod -x time -y packet temp.rands &
#	exec xgraph -bb -tk -nl -m -x time -y packet temp.rands &
	
	exit 0
}

proc openTrace { stopTime testName mod } {
	exec rm -f out.tr temp.rands
	global r1 k1    
	set traceFile [open out.tr w]
	ns at $stopTime \
		"close $traceFile ; finish $testName $mod"
	set T [ns trace]
	$T attach $traceFile
	return $T
}


proc openTraces { stopTime testName filename node1 node2 mod } {
        global s1 s2 r1 r2 k1
        exec rm -f $filename temp.rands
        set traceFile [open $filename w]
        ns at $stopTime \
                "close $traceFile"
        set T [ns trace]
        $T attach $traceFile
        [ns link $node1 $node2] trace $T
}

proc openTraces1 { stopTime testName filename node1 node2 mod } {
        global s1 s2 r1 r2 k1
        exec rm -f $filename temp.rands
        set traceFile [open $filename w]
        ns at $stopTime \
                "close $traceFile ; finish $testName $mod"
        set T1 [ns trace]
        $T1 attach $traceFile
        [ns link $node1 $node2] trace $T1
}

proc test_one_drop {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set mod 60
	set endtime 6.0
	[ns link $r2 $k1] set queue-limit 8

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set ftp1 [$tcp1 source ftp]

	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 8
	set ftp2 [$tcp2 source ftp]
	$ftp2 set maxpkts 12

	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 8
	set ftp3 [$tcp3 source ftp]
	$ftp3 set maxpkts 10

	ns at 1.0 "$ftp1 start"
	ns at 1.2 "$ftp2 start"
	ns at 1.2 "$ftp3 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_one_drop out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_one_drop out.tr1 $r2 $r1 $mod

	ns run
}

proc test_two_drops {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set mod 60
	set endtime 6.0
	[ns link $r2 $k1] set queue-limit 8

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set ftp1 [$tcp1 source ftp]

	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 8
	set ftp2 [$tcp2 source ftp]
	$ftp2 set maxpkts 12

	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 8
	set ftp3 [$tcp3 source ftp]
	$ftp3 set maxpkts 11

	ns at 1.0 "$ftp1 start"
	ns at 1.2 "$ftp2 start"
	ns at 1.2 "$ftp3 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_two_drops out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_two_drops out.tr1 $r2 $r1 $mod

	ns run
}

proc test_three_drops {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set mod 60
	set endtime 6.0
	[ns link $r2 $k1] set queue-limit 8

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set ftp1 [$tcp1 source ftp]

	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 8
	set ftp2 [$tcp2 source ftp]
	$ftp2 set maxpkts 12

	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 8
	set ftp3 [$tcp3 source ftp]
	$ftp3 set maxpkts 12

	ns at 1.0 "$ftp1 start"
	ns at 1.2 "$ftp2 start"
	ns at 1.18 "$ftp3 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_three_drops out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_three_drops out.tr1 $r2 $r1 $mod

	ns run
}

proc test_four_drops {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set mod 60
	set endtime 6.0
	[ns link $r2 $k1] set queue-limit 8

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set ftp1 [$tcp1 source ftp]

	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 8
	set ftp2 [$tcp2 source ftp]
	$ftp2 set maxpkts 12

	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 8
	set ftp3 [$tcp3 source ftp]
	$ftp3 set maxpkts 13

	ns at 1.0 "$ftp1 start"
	ns at 1.2 "$ftp2 start"
	ns at 1.2 "$ftp3 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_four_drops out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_four_drops out.tr1 $r2 $r1 $mod

	ns run
}

proc test_many_drops {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set mod 60
	set endtime 6.0
	[ns link $r2 $k1] set queue-limit 8

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	set ftp1 [$tcp1 source ftp]

	set tcp2 [ns_create_connection tcp-reno $s1 tcp-sink $k1 1]
	$tcp2 set window 8
	set ftp2 [$tcp2 source ftp]
	$ftp2 set maxpkts 12

	set tcp3 [ns_create_connection tcp-reno $s1 tcp-sink $k1 2]
	$tcp3 set window 8
	set ftp3 [$tcp3 source ftp]
	$ftp3 set maxpkts 16

	ns at 1.0 "$ftp1 start"
	ns at 1.2 "$ftp2 start"
	ns at 1.2 "$ftp3 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_many_drops out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_many_drops out.tr1 $r2 $r1 $mod

	ns run
}

#
# In the following tests, the TCP connection has two related parameters,
# "window", for the receiver's advertised window, and "maxcwnd",
# for the max congestion window.
#
proc test_one_14 {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 5.0

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 28
	$tcp1 set maxcwnd 14
	set ftp1 [$tcp1 source ftp]

	ns at 1.0 "$ftp1 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_one_14 out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_one_14 out.tr1 $r2 $r1 $mod

	ns run
}

proc test_one_15 {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 5.0

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 30
	$tcp1 set maxcwnd 15
	set ftp1 [$tcp1 source ftp]

	ns at 1.0 "$ftp1 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_one_15 out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_one_15 out.tr1 $r2 $r1 $mod

	ns run
}

proc test_one_20 {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 10.0

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 40
	$tcp1 set maxcwnd 20
	set ftp1 [$tcp1 source ftp]

	ns at 1.0 "$ftp1 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_one_20 out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_one_20 out.tr1 $r2 $r1 $mod

	ns run
}

proc test_one_26 {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 10.0

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 52 
	$tcp1 set maxcwnd 26
	set ftp1 [$tcp1 source ftp]

	ns at 1.0 "$ftp1 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_one_26 out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_one_26 out.tr1 $r2 $r1 $mod

	ns run
}

# delayed ack not implemented for sack.
# proc test_one_40 {} { 
#         global mod s1 s2 r1 r2 r2 s3 s4
#         create_testnet2
#         [ns link $r1 $r2] set queue-limit 29
#         set tcp1 [ns_create_connection tcp $s1 tcp-sink-da $r2 0]
#         $tcp1 set window 40
# 
#         set ftp1 [$tcp1 source ftp]
#         ns at 0.0 "$ftp1 start"
# 
#         # trace only the bottleneck link
# 	openTraces 5.0 test_type_one_40 out $r1 $r2 $k1
# 
#         ns run
# }

proc test_two_2 {} {
	global mod s1 s2 r1 r2 k1 
	create_testnet
	set endtime 8.0

	[ns link $r2 $k1] set queue-limit 8
	[ns link $k1 $r2] set queue-limit 8
	
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 200
	$tcp1 set maxcwnd 100

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 32
	$tcp2 set window 16

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 0.5 "$ftp2 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_two_2 out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_two_2 out.tr1 $r2 $r1 $mod

	ns run
}

proc test_two_A {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 10.0
	[ns link $r2 $k1] set queue-limit 9

	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 100
	$tcp1 set maxcwnd 50

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 40
	$tcp2 set maxcwnd 20

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 1.0 "$ftp1 start"
	ns at 1.0 "$ftp2 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_two_A out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_two_A out.tr1 $r2 $r1 $mod

	ns run
}

proc test_two_B {} {
	global mod s1 s2 r1 r2 k1
	create_testnet
	set endtime 10.0
	[ns link $s2 $r1] set delay 200ms
	[ns link $r1 $s2] set delay 200ms
	[ns link $r2 $k1] set queue-limit 11
	[ns link $k1 $r2] set queue-limit 11
	
	set tcp1 [ns_create_connection tcp $s1 tcp-sink $k1 0]
	$tcp1 set window 60
	$tcp1 set maxcwnd 30 

	set tcp2 [ns_create_connection tcp $s2 tcp-sink $k1 1]
	$tcp2 set window 60
	$tcp2 set maxcwnd 30

	set ftp1 [$tcp1 source ftp]
	set ftp2 [$tcp2 source ftp]

	ns at 0.0 "$ftp1 start"
	ns at 0.0 "$ftp2 start"

	# trace only the bottleneck link
	openTraces $endtime test_type_two_B out.tr $r2 $k1 $mod
	openTraces1 $endtime test_type_two_B out.tr1 $r2 $r1 $mod

	ns run
}

if { $argc != 1 } {
	puts stderr {usage: ns test-suite.tcl [ 1 2 3 ... ]}
	exit 1
}
if { "[info procs test_$argv]" != "test_$argv" } {
	puts stderr "sack.tcl: no such test: $argv"
}
test_$argv

