# Copyright (c) 1995 The Regents of the University of California.
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
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

# test-suite-full-fixes.tcl
#
# Based on ns/tcl/test/test-suite-full.tcl
#
# Modified June 2005 by Michele C. Weigle, Clemson University
#
# For a detailed description of the examples in this file, see
# http://www.cs.clemson.edu/~mweigle/research/ns/fixes/
#

source misc.tcl
source topologies-full-fixes.tcl

# defaults as of ns-2.1b9  May 2002
Agent/TCP set tcpTick_ 0.01
Agent/TCP set rfc2988_ true
Agent/TCP set useHeaders_ true
Agent/TCP set windowInit_ 2
Agent/TCP set minrto_ 1
Agent/TCP set singledup_ 1

Trace set show_tcphdr_ 1 ; # needed to plot ack numbers for tracing 

# Agent/TCP/FullTcp set debug_ true;

TestSuite instproc finish testname {
	$self instvar ns_
	$ns_ halt

        set awkCode {
	    {if (($1=="+" || $1=="d") && (($3=="3" && $4=="2") || ($3=="2" && $4=="3"))) print}
	}
        exec awk $awkCode all.tr > temp.rands
}

#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#                         TCP Reno Tests
#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#*************************************************************************
# Tests to validate bug fixes to Full-TCP Reno in tcp-full.cc
#
#  reno-drop-passive-fin -- drop the passive FIN
#  reno-drop-synack -- drop the SYN/ACK
#  reno-msg_eof-fin -- send message with MSG_EOF set
#  reno-drop-lastack -- drop the last ACK in connection teardown
#
#*************************************************************************

#
# reno-drop-passive-fin
#
Class Test/reno-drop-passive-fin -superclass TestSuite
Test/reno-drop-passive-fin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy2
	set test_ reno-drop-passive-fin
	$self next
}
Test/reno-drop-passive-fin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 5.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "reno-drop-passive-fin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 7.0
	$errmodel set burstlen_ 2

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$ns_ at 0.7 "$src advance 5"
	$ns_ at 2.0 "$src close"

	# set up special params for this test
	$src set window_ 20
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# reno-drop-synack
#
Class Test/reno-drop-synack -superclass TestSuite
Test/reno-drop-synack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy2;    # drops are all ACKs
	set test_ reno-drop-synack
	$self next
}
Test/reno-drop-synack instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 7

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "reno-drop-synack: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 1.0
	$errmodel set burstlen_ 1

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# reno-msg_eof-fin
#
Class Test/reno-msg_eof-fin -superclass TestSuite
Test/reno-msg_eof-fin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0
	set test_ reno-msg_eof-fin
	$self next
}
Test/reno-msg_eof-fin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 3.0	

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
        $ns_ at 0.7 "$src advanceby 5"
	$ns_ at 1.3 "$src sendmsg 400 MSG_EOF"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# reno-drop-lastack
#
Class Test/reno-drop-lastack -superclass TestSuite
Test/reno-drop-lastack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy;
	set test_ reno-drop-lastack
	$self next
}
Test/reno-drop-lastack instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 50.0

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "reno-drop-lastack: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 7.0
	$errmodel set burstlen_ 1
	$errmodel set period_ 1000

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$ns_ at 0.7 "$src advanceby 3"
	$ns_ at 2.0 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#                         ECN Tests
#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#*************************************************************************
# Tests to validate bug fixes to Full-TCP ECN in tcp-full.cc
#
#  ecn-drop-synack -- drop the SYN/ACK
#  ecn-rexmt -- retransmit a packet
#
#*************************************************************************

#
# ecn-rexmt
#
Class Test/ecn-rexmt -superclass TestSuite
Test/ecn-rexmt instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy;
	set test_ ecn-rexmt
	$self next
}
Test/ecn-rexmt instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 1.6

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "reno-rexmt: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 5.0
	$errmodel set burstlen_ 1

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576
	$src set ecn_ true
	$sink set ecn_ true

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# ecn-drop-synack
#
Class Test/ecn-drop-synack -superclass TestSuite
Test/ecn-drop-synack instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy2;    # drops are all ACKs
	set test_ ecn-drop-synack
	$self next
}
Test/ecn-drop-synack instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 7

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "ecn-drop-synack: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 1.0
	$errmodel set burstlen_ 1

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp]
	set sink [new Agent/TCP/FullTcp]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576
	$src set ecn_ true
	$sink set ecn_ true

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}


#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#                         TCP SACK Tests
#:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#*************************************************************************
# Tests to validate bug fixes to Full-TCP SACK in tcp-full.cc
#
#  sack-send-past-fin -- as long as cwnd allows, sender sends packets even
#                        when there is no new data
#  sack-ecn-drop-mark -- SACK behavior when seeing 3 DUPACKS after ECN
#  sack-burst -- SACK sends large number of old packets
#  sack-burst-2 -- SACK sends large number of old packets
#  sack-illegal-sack-block -- dropped FIN
#  sack-dropwin -- drop a full window of packets
#*************************************************************************

#
# sack-send-past-fin
#
Class Test/sack-send-past-fin -superclass TestSuite
Test/sack-send-past-fin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ sack-send-past-fin
	$self next
}
Test/sack-send-past-fin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 3.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-send-past-fin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 11.0
	$errmodel set burstlen_ 1

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.5 "$ftp1 stop"

	# set up special params for this test
	$src set window_ 20
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# sack-ecn-drop-mark
#
# This test is to illustrate the bug in the Full-TCP SACK+ECN 
# implementation that occurs when a packet loss is detected
# in the same window of ACKs as an ECN-Echo bit.

Class Test/sack-ecn-drop-mark -superclass TestSuite
Test/sack-ecn-drop-mark instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ sack-ecn-drop-mark
	$self next
}
Test/sack-ecn-drop-mark instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 2.1

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-ecn: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 20.0
	$errmodel set period_ 1000.0
	$errmodel set markecn_ true; # mark ecn's, don't drop

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	$ns_ at 0.7 "$src advance 40"

	# set up special params for this test
	$src set window_ 70
	$src set packetSize_ 576

	$src set ecn_ true
	$sink set ecn_ true

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# sack-burst
#
Class Test/sack-burst -superclass TestSuite
Test/sack-burst instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net1-lossy
	set test_ sack-burst
	$self next
}
Test/sack-burst instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 23

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-burst: confused by >1 err models..abort"
		exit 1
	}

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0

	# set up TCP-level connections

	# flow 1 -- one drop early
	$ns_ connect $src $sink
	$sink listen
	$ns_ at 0.0 "$errmodel set offset_ 5"
	$ns_ at 0.0 "$errmodel set burstlen_ 1"
	$ns_ at 0.0 "$errmodel set period_ 10000"
	$ns_ at 0.7 "$src advanceby 10"
	$ns_ at 2.0 "$src close"

	# flow 2 -- no drops, send more than flow 1, last pckt 
	# smaller than full-size
	$ns_ at 2.5 "$ns_ connect $src $sink"
	$ns_ at 2.6 "$sink listen"
	$ns_ at 3.0 "$src advance-bytes 1702"
	$ns_ at 7.0 "$src close"

	# flow 3 -- one drop late, TIMEOUT
	$ns_ at 19.5 "$errmodel set burstlen_ 1"
	$ns_ at 19.5 "$errmodel set period_ 52.0"
	$ns_ at 20.0 "$ns_ connect $src $sink"
	$ns_ at 20.1 "$sink listen"
	$ns_ at 20.3 "$src advanceby 48"
	$ns_ at 21.372 "$errmodel set period_ 2.0"
	$ns_ at 21.380 "$errmodel set burstlen_ 11"
	$ns_ at 21.381 "$errmodel set period_ 1000.0"
	$ns_ at 30.0 "$src close"

	# set up special params for this test
	$src set window_ 20
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}


#
# sack-burst-2
#
Class Test/sack-burst-2 -superclass TestSuite
Test/sack-burst-2 instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net1-lossy
	set test_ sack-burst-2
	$self next
}
Test/sack-burst-2 instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 3.5

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-burst-2: confused by >1 err models..abort"
		exit 1
	}

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0

	# set up TCP-level connection

	# one drop early, SACK recovery
	$ns_ connect $src $sink
	$sink listen
	$ns_ at 0.0 "$errmodel set offset_ 5"
	$ns_ at 0.0 "$errmodel set burstlen_ 1"
	$ns_ at 0.0 "$errmodel set period_ 10000"
	$ns_ at 0.7 "$src advanceby 40"

        # cause timeout
	$ns_ at 2.0 "$errmodel set period_ 2.0"
	$ns_ at 2.0 "$errmodel set burstlen_ 1"
	$ns_ at 2.5 "$errmodel set period_ 1000.0"

	$ns_ at $stopt "$src close"

	# set up special params for this test
	$src set window_ 20
	$src set packetSize_ 576

	$self traceQueues $node_(s1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# sack-illegal-sack-block
#
Class Test/sack-illegal-sack-block -superclass TestSuite
Test/sack-illegal-sack-block instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ sack-illegal-sack-block
	$self next
}
Test/sack-illegal-sack-block instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 2.0	

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-illegal-sack-block: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 8.0
	$errmodel set burstlen_ 1

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"
	$ns_ at 1.3 "$src close"

	# set up special params for this test
	$src set window_ 100
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
}

#
# sack-dropwin
#
Class Test/sack-dropwin -superclass TestSuite
Test/sack-dropwin instproc init topo {
	$self instvar net_ defNet_ test_
	set net_ $topo
	set defNet_ net0-lossy
	set test_ sack-dropwin
	$self next
}
Test/sack-dropwin instproc run {} {
	$self instvar ns_ node_ testName_ topo_

	set stopt 5.5

	$topo_ instvar lossylink_
	set errmodule [$lossylink_ errormodule]
	set errmodel [$errmodule errormodels]
	if { [llength $errmodel] > 1 } {
		puts "sack-dropwin: confused by >1 err models..abort"
		exit 1
	}

	$errmodel set offset_ 47.0
	$errmodel set burstlen_ 10
	$errmodel set period_ 1000

	# set up connection (do not use "create-connection" method because
	# we need a handle on the sink object)
	set src [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_(s1) $src
	$ns_ attach-agent $node_(k1) $sink
	$src set fid_ 0
	$sink set fid_ 0
	$ns_ connect $src $sink

	# set up TCP-level connections
	$sink listen
	set ftp1 [$src attach-app FTP]
	$ns_ at 0.7 "$ftp1 start"

	# set up special params for this test
	$src set window_ 10
	$src set packetSize_ 576

	$self traceQueues $node_(r1) [$self openTrace $stopt $testName_]
	$ns_ run
} 

TestSuite runTest
