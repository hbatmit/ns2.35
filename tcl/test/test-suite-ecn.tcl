#
# Copyright (c) 1995-1997 The Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-ecn.tcl,v 1.47 2010/04/03 20:40:15 tom_henderson Exp $
#
# To run all tests: test-all-ecn

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
catch "cd $dir"

# FOR UPDATING GLOBAL DEFAULTS:
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Agent/TCPSink set SYN_immediate_ack_ false
# default changed 02/2010.


set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Topology instproc makenodes ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]
    set node_(r3) [$ns node]
}

Topology instproc createlinks ns {  
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
}

Topology instproc createlinks3 ns {  
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(r2) $node_(r3) 100Mb 0ms RED
    $ns queue-limit $node_(r2) $node_(r3) 100
    $ns queue-limit $node_(r3) $node_(r2) 100
    $ns duplex-link $node_(s3) $node_(r3) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r3) 10Mb 5ms DropTail

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r3) queuePos 0
    $ns duplex-link-op $node_(r3) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r3) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r3) orient left-up
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks $ns
}

Class Topology/net2-lossy -superclass Topology
Topology/net2-lossy instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks $ns
 
    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
} 

Class Topology/net3-lossy -superclass Topology
Topology/net3-lossy instproc init ns {
    $self instvar node_
    $self makenodes $ns
    $self createlinks3 $ns
 
    $self instvar lossylink_
    set lossylink_ [$ns link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass

    $self instvar lossylink1_
    set lossylink1_ [$ns link $node_(r2) $node_(r3)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink1_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
} 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_ cwnd_chan_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	   $PERL ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp.rands
	exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	  $PERL ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp1.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
# The line below plots both data and ack packets.
#        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
#                     temp1.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
	}
	if { [info exists cwnd_chan_] && $quiet == "false" } {
		$self plot_cwnd 
	}
	$ns_ halt
}

TestSuite instproc enable_tracequeue ns {
	$self instvar tchan_ node_
	set redq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$redq trace curq_
	$redq trace ave_
	$redq attach $tchan_
}

TestSuite instproc plotQueue file {
	global quiet
	$self instvar tchan_
	#
	# Plot the queue size and average queue size, for RED gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
			else if ($1 == "a" && NF>2)
				print $2, $3 >> "temp.a";
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q temp.a 
	exec touch temp.a temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
	puts $f \n\"ave_queue
	exec cat temp.a >@ $f
	###puts $f \n"thresh
	###puts $f 0 [[ns link $r1 $r2] get thresh]
	###puts $f $end [[ns link $r1 $r2] get thresh]
	close $f
	if {$quiet == "false"} {
		exec xgraph -bb -tk -x time -y queue temp.queue &
	}
}

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
    global quiet
    $self instvar dump_inst_ ns_
    if ![info exists dump_inst_($tcpSrc)] {
	set dump_inst_($tcpSrc) 1
	set report $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]
	if {$quiet == "false"} {
		puts $report
	}
	$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
	return
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
    set report time=[$ns_ now]/class=$label/ack=[$tcpSrc set ack_]/packets_resent=[$tcpSrc set nrexmitpack_]
    if {$quiet == "false"} {
    	puts $report
    }
}       

TestSuite instproc emod {} {
	$self instvar topo_
	$topo_ instvar lossylink_ 
        set errmodule [$lossylink_ errormodule]
	return $errmodule
}

TestSuite instproc emod2 {} {
	$self instvar topo_
	$topo_ instvar lossylink1_ 
        set errmodule [$lossylink1_ errormodule]
	return $errmodule
}

TestSuite instproc setloss {} {
	$self instvar topo_
	$topo_ instvar lossylink_
        set errmodule [$lossylink_ errormodule]
        set errmodel [$errmodule errormodels]
        if { [llength $errmodel] > 1 } {
                puts "droppedfin: confused by >1 err models..abort"
                exit 1
        }
	return $errmodel
}

Class Test/ecn -superclass TestSuite
Test/ecn instproc init {} {
    $self instvar net_ test_ guide_
    Queue/RED set setbit_ true
    set net_	net2
    set test_	ecn
    set guide_ "Two connections using ECN."
    $self next pktTraceFile
}
Test/ecn instproc run {} {
    global quiet
    $self instvar ns_ node_ testName_ guide_
    puts "Guide: $guide_"
    $self setTopo 

    set stoptime 10.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true

    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    $tcp1 set ecn_ 1

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15
    $tcp2 set ecn_ 1
        
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
        
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_" 
        
    $ns_ run
}

#######################################################################

TestSuite instproc ecnsetup { tcptype {stoptime 3.0} { tcp1fid 0 } { delack 0 }} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
##
##  Agent/TCP set maxburst_ 4
##
    set delay 10ms
    $ns_ delay $node_(r1) $node_(r2) $delay
    $ns_ delay $node_(r2) $node_(r1) $delay

    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set setbit_ true

    if {$tcptype == "Tahoe" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    } elseif {$tcptype == "Sack1" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
	  TCPSink/Sack1  $node_(s3) $tcp1fid]
    } elseif {$delack == 0} {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
	  TCPSink $node_(s3) $tcp1fid]
    } elseif {$tcptype == "Tahoe" && $delack == 1} {
      set tcp1 [$ns_ create-connection TCP $node_(s1) \
	  TCPSink/DelAck $node_(s3) $tcp1fid]
    } elseif {$tcptype == "Sack1" && $delack == 1} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) \
	  TCPSink/Sack1/DelAck  $node_(s3) $tcp1fid]
    } else {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_(s1) \
	  TCPSink/DelAck $node_(s3) $tcp1fid]
    } 
    $tcp1 set window_ 30
    $tcp1 set ecn_ 1
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
        
    #$self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
        
    $self tcpDump $tcp1 5.0
        
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_" 
}

TestSuite instproc second_tcp { tcptype starttime } {
    $self instvar ns_ node_ 
    if {$tcptype == "Tahoe"} {
      set tcp [$ns_ create-connection TCP $node_(s1) \
	 TCPSink $node_(s3) 2]    
    } elseif {$tcptype == "Sack1"} { 
      set tcp [$ns_ create-connection TCP/Sack1 $node_(s1) \
          TCPSink/Sack1  $node_(s3) 2]
    } else {
      set tcp [$ns_ create-connection TCP/$tcptype $node_(s1) \
	 TCPSink $node_(s3) 2]
    }
    $tcp set window_ 30
    $tcp set ecn_ 1
    set ftp [$tcp attach-app FTP]
    $ns_ at $starttime "$ftp start"
}

# Drop the specified packet.
TestSuite instproc drop_pkt { number } {
    $self instvar ns_ lossmodel
    set lossmodel [$self setloss]
    $lossmodel set offset_ $number
    $lossmodel set period_ 10000
}

TestSuite instproc drop_pkts pkts {
    $self instvar ns_ errmodel1
    set emod [$self emod]
    set errmodel1 [new ErrorModel/List]
    $errmodel1 droplist $pkts
    $emod insert $errmodel1
    $emod bind $errmodel1 1

}

TestSuite instproc drop_pkts2 pkts {
    $self instvar ns_ errmodel2
    set emod [$self emod2]
    set errmodel2 [new ErrorModel/List]
    $errmodel2 droplist $pkts
    $emod insert $errmodel2
    $emod bind $errmodel2 1
}

#######################################################################
# Tahoe Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_tahoe -superclass TestSuite
Test/ecn_nodrop_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_tahoe
        set guide_	"Tahoe with ECN."
        $self next pktTraceFile
}
Test/ecn_nodrop_tahoe instproc run {} {
	global quiet
	$self instvar ns_ guide_
	puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_tahoe -superclass TestSuite
Test/ecn_twoecn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_tahoe
	set guide_ 	"Tahoe, two marked packets in one window."
        $self next pktTraceFile
}
Test/ecn_twoecn_tahoe instproc run {} {
        global quiet
	$self instvar ns_ lossmodel guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_tahoe -superclass TestSuite
Test/ecn_drop_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_tahoe
        set guide_	"Tahoe, ECN followed by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0
	$self drop_pkt 195
	$ns_ run
}

# ECN preceded by packet loss.
Class Test/ecn_drop1_tahoe -superclass TestSuite
Test/ecn_drop1_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_tahoe
        set guide_      "Tahoe, ECN preceded by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop1_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0
	$self drop_pkt 185
	$ns_ run
}

# Packet loss only.
Class Test/ecn_noecn_tahoe -superclass TestSuite
Test/ecn_noecn_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 1000
	Queue/RED set maxthresh_ 1000
        set net_	net2-lossy
        set test_	ecn_noecn_tahoe
	Test/ecn_noecn_tahoe instproc run {} [Test/ecn_drop_tahoe info instbody run ]
        set guide_      "Tahoe, packet loss only, no ECN."
        $self next pktTraceFile
}

# Multiple dup acks 
Class Test/ecn_bursty_tahoe -superclass TestSuite
Test/ecn_bursty_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
        set test_	ecn_bursty_tahoe
        set guide_      "Tahoe, multiple dup acks."
        $self next pktTraceFile
}
Test/ecn_bursty_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"

	$self ecnsetup Tahoe 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 189
	$lossmodel set burstlen_ 15
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn_tahoe -superclass TestSuite
Test/ecn_burstyEcn_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_burstyEcn_tahoe
	Test/ecn_burstyEcn_tahoe instproc run {} [Test/ecn_bursty_tahoe info instbody run ]   
        set guide_      "Tahoe, multiple dup acks and ECN."
        $self next pktTraceFile
}

# Multiple dup acks without bugFix_
Class Test/ecn_noBugfix_tahoe -superclass TestSuite
Test/ecn_noBugfix_tahoe instproc init {} {
        $self instvar net_ test_ guide_
	Queue/RED set thresh_ 100 
	Queue/RED set maxthresh_ 100
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ false
        set test_	ecn_noBugfix_tahoe
	Test/ecn_noBugfix_tahoe instproc run {} [Test/ecn_bursty_tahoe info instbody run ]

        set guide_      "Tahoe, multiple dup acks, no bugFix."
        $self next pktTraceFile
}

# ECN followed by timeout.
Class Test/ecn_timeout_tahoe -superclass TestSuite
Test/ecn_timeout_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout_tahoe
        set guide_      "Tahoe, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 1
	$self drop_pkts {242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268} 

	$ns_ run
}

# Only the timeout.
Class Test/ecn_timeout2_tahoe -superclass TestSuite
Test/ecn_timeout2_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
        set test_	ecn_timeout2_tahoe
        set guide_      "Tahoe, drops followed by a timeout, no ECN."
        $self next pktTraceFile
}
Test/ecn_timeout2_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 1
	$self drop_pkts {241 242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270 271 272} 

	$ns_ run
}

# The timeout with the ECN in the middle of dropped packets.
Class Test/ecn_timeout3_tahoe -superclass TestSuite
Test/ecn_timeout3_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout3_tahoe
        set guide_      "Tahoe, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout3_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 1
	$self drop_pkts {241 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270} 

	$ns_ run
}

# Packet drops with a window of one packet.
Class Test/ecn_smallwin_tahoe -superclass TestSuite
Test/ecn_smallwin_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Agent/TCP set timerfix_ false
	# The default is being changed to true.
        set net_	net2-lossy
        set test_	ecn_smallwin_tahoe
        set guide_      "Tahoe, packet drops with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwin_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 6.0 1
	$self drop_pkts {4 8 9 10 11 12 13 100 115 118 119}

	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_tahoe -superclass TestSuite
Test/ecn_smallwinEcn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set timerfix_ false
	# The default is being changed to true.
        set test_	ecn_smallwinEcn_tahoe
        set guide_      "Tahoe, ECN with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 10.0 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 
	$errmodel1 set markecn_ true
	$ns_ run
}

# ECN, cwnd 4, packet 4 is dropped, and then packet 6 is marked. 
# The retransmit timer expires, packet 4 is retransmitted, and then the
#   ECN bit is set on the retransmitted packet.
# When the ACK for packet 4 comes in, the retransmit timer must not get 
#   cancelled.

Class Test/ecn_smallwin1Ecn_tahoe -superclass TestSuite
Test/ecn_smallwin1Ecn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net3-lossy
	Agent/TCP set timerfix_ false
	# The default is being changed to true.
        set test_	ecn_smallwin1Ecn_tahoe
        set guide_      "Tahoe, ECN with small windows."
        $self next pktTraceFile
}
Test/ecn_smallwin1Ecn_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1 errmodel2
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 10.0 1
	$self drop_pkts {4 8 9 11 12 13 120 135 143 148 150 153} 
	$errmodel1 set markecn_ true
	$self drop_pkts2 {6}
	$errmodel2 set markecn_ false
	$ns_ run
}

# ECN with a window of one packet, slow_start_restart_ false.
Class Test/ecn_smallwin2Ecn_tahoe -superclass TestSuite
Test/ecn_smallwin2Ecn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set slow_start_restart_ false
	Agent/TCP set timerfix_ false
	# The default is being changed to true.
        set test_	ecn_smallwin2Ecn_tahoe
	Test/ecn_smallwin2Ecn_tahoe instproc run {} [Test/ecn_smallwinEcn_tahoe info instbody run ]
        set guide_      "Tahoe, ECN with a window of one, no slow_start_restart."
        $self next pktTraceFile
}

Class Test/ecn_smallwin3Ecn_tahoe -superclass TestSuite
Test/ecn_smallwin3Ecn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net3-lossy
	Agent/TCP set slow_start_restart_ false
        set test_	ecn_smallwin3Ecn_tahoe
	Test/ecn_smallwin3Ecn_tahoe instproc run {} [Test/ecn_smallwin1Ecn_tahoe info instbody run ]
        set guide_      "Tahoe, ECN with small windows, no slow_start_restart."
        $self next pktTraceFile
}

# Packet drops for the second packet.
Class Test/ecn_secondpkt_tahoe -superclass TestSuite
Test/ecn_secondpkt_tahoe instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpkt_tahoe
        set guide_      "Tahoe, with the second packet dropped."
        $self next pktTraceFile
}
Test/ecn_secondpkt_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 2.0 1
	$self drop_pkts {1 3} 

	$ns_ run
}

# ECN for the second packet.
Class Test/ecn_secondpktEcn_tahoe -superclass TestSuite
Test/ecn_secondpktEcn_tahoe instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpktEcn_tahoe
        set guide_      "Tahoe, with the second packet marked."
        $self next pktTraceFile
}
Test/ecn_secondpktEcn_tahoe instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 2.0 1
	$self drop_pkts {1 3} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Delayed Ack Tahoe Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_tahoe_delack -superclass TestSuite
Test/ecn_nodrop_tahoe_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_tahoe_delack
        set guide_      "Tahoe with ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_nodrop_tahoe_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 0 1
	$self drop_pkt 10000
	$ns_ run
}

# This one doesn't have two ECNs close together..
Class Test/ecn_twoecn_tahoe_delack -superclass TestSuite
Test/ecn_twoecn_tahoe_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_tahoe_delack
        set guide_      "Tahoe, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_twoecn_tahoe_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 0 1 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss?
Class Test/ecn_drop_tahoe_delack -superclass TestSuite
Test/ecn_drop_tahoe_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_tahoe_delack
        set guide_      "Tahoe, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_drop_tahoe_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 0 1
	$self drop_pkt 195
	$ns_ run
}

# ECN preceded by packet loss?
Class Test/ecn_drop1_tahoe_delack -superclass TestSuite
Test/ecn_drop1_tahoe_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_tahoe_delack
        set guide_      "Tahoe, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_drop1_tahoe_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 3.0 0 1
	$self drop_pkt 185
	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_tahoe_delack -superclass TestSuite
Test/ecn_smallwinEcn_tahoe_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set timerfix_ false
	# The default is being changed to true.
        set test_	ecn_smallwinEcn_tahoe_delack
        set guide_      "Tahoe, ECN with a window of one, delayed acks."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_tahoe_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Tahoe 10.0 1 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Reno Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_reno -superclass TestSuite
Test/ecn_nodrop_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_reno
 	set guide_      "Reno with ECN."  
        $self next pktTraceFile
}
Test/ecn_nodrop_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_reno -superclass TestSuite
Test/ecn_twoecn_reno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_reno
        set guide_      "Reno, two marked packets in one window."
        $self next pktTraceFile
}
Test/ecn_twoecn_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_reno -superclass TestSuite
Test/ecn_drop_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_reno
        set guide_      "Reno, ECN followed by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0
	$self drop_pkt 195
	$ns_ run
}

# ECN preceded by packet loss.
Class Test/ecn_drop1_reno -superclass TestSuite
Test/ecn_drop1_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_reno
        set guide_      "Reno, ECN preceded by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop1_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0
	$self drop_pkt 185
	$ns_ run
}

# Packet loss only.
Class Test/ecn_noecn_reno -superclass TestSuite
Test/ecn_noecn_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 1000
	Queue/RED set maxthresh_ 1000
        set net_	net2-lossy
        set test_	ecn_noecn_reno
	Test/ecn_noecn_reno instproc run {} [Test/ecn_drop_reno info instbody run ]
        set guide_      "Reno, packet loss only, no ECN."
        $self next pktTraceFile
}

# Multiple dup acks 
Class Test/ecn_bursty_reno -superclass TestSuite
Test/ecn_bursty_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
        set test_	ecn_bursty_reno
        set guide_      "Reno, multiple dup acks."
        $self next pktTraceFile
}
Test/ecn_bursty_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"

	$self ecnsetup Reno 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 189
	$lossmodel set burstlen_ 15
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn_reno -superclass TestSuite
Test/ecn_burstyEcn_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_burstyEcn_reno
	Test/ecn_burstyEcn_reno instproc run {} [Test/ecn_bursty_reno info instbody run ]   
        set guide_      "Reno, multiple dup acks and ECN."
        $self next pktTraceFile
}

# Multiple dup acks without bugFix_
Class Test/ecn_noBugfix_reno -superclass TestSuite
Test/ecn_noBugfix_reno instproc init {} {
        $self instvar net_ test_ guide_
	Queue/RED set thresh_ 100 
	Queue/RED set maxthresh_ 100
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ false
        set test_	ecn_noBugfix_reno
	Test/ecn_noBugfix_reno instproc run {} [Test/ecn_bursty_reno info instbody run ]
        set guide_      "Reno, multiple dup acks, no bugFix."
        $self next pktTraceFile
}

# Drops and a timeout.
Class Test/ecn_timeout_reno -superclass TestSuite
Test/ecn_timeout_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ false
        set net_	net2-lossy
        set test_	ecn_timeout_reno
        set guide_      "Reno, ECN and drops followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 1
	$self drop_pkts {242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268} 

	$ns_ run
}

# ECN followed by a timeout, followed by an ECN representing a
# new instance of congestion.
Class Test/ecn_timeout1_reno -superclass TestSuite
Test/ecn_timeout1_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout1_reno
        set guide_      "Reno, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout1_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 1
	$self drop_pkts {245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265} 
	$self second_tcp Tahoe 1.0
	$ns_ run
}

# Packet drops with a window of one packet.
Class Test/ecn_smallwin_reno -superclass TestSuite
Test/ecn_smallwin_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwin_reno
        set guide_      "Reno, packet drops with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwin_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 6.0 1
	$self drop_pkts {4 8 9 10 11 12 13 100 115 118 119 121 122}

	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_reno -superclass TestSuite
Test/ecn_smallwinEcn_reno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_reno
        set guide_      "Reno, ECN with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Reno 10.0 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 
	$errmodel1 set markecn_ true
	$ns_ run
}

Class Test/ecn_smallwin1Ecn_reno -superclass TestSuite
Test/ecn_smallwin1Ecn_reno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net3-lossy
        set test_	ecn_smallwin1Ecn_reno
        set guide_      "Reno, ECN with small windows."
        $self next pktTraceFile
}
Test/ecn_smallwin1Ecn_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1 errmodel2
        puts "Guide: $guide_"
	$self ecnsetup Reno 10.0 1
	$self drop_pkts {4 8 9 11 12 13 120 135 143 148 150 153} 
	$errmodel1 set markecn_ true
	$self drop_pkts2 {6}
	$errmodel2 set markecn_ false
	$ns_ run
}

# Packet drops for the second packet.
Class Test/ecn_secondpkt_reno -superclass TestSuite
Test/ecn_secondpkt_reno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpkt_reno
        set guide_      "Reno, with the second packet dropped."
        $self next pktTraceFile
}
Test/ecn_secondpkt_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 2.0 1
	$self drop_pkts {1 3} 

	$ns_ run
}

# ECN for the second packet.
Class Test/ecn_secondpktEcn_reno -superclass TestSuite
Test/ecn_secondpktEcn_reno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpktEcn_reno
        set guide_      "Reno, with the second packet marked."
        $self next pktTraceFile
}
Test/ecn_secondpktEcn_reno instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Reno 2.0 1
	$self drop_pkts {1 3} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Delayed Ack Reno Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_reno_delack -superclass TestSuite
Test/ecn_nodrop_reno_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_reno_delack
        set guide_      "Reno, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_nodrop_reno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 0 1 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_reno_delack -superclass TestSuite
Test/ecn_twoecn_reno_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_reno_delack
        set guide_      "Reno, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_twoecn_reno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 0 1 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_reno_delack -superclass TestSuite
Test/ecn_drop_reno_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_reno_delack
        set guide_      "Reno, packet loss followed by ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_drop_reno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Reno 3.0 0 1
	$self drop_pkt 195
	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_reno_delack -superclass TestSuite
Test/ecn_smallwinEcn_reno_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_reno_delack
        set guide_      "Reno, delayed acks, ECN with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_reno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Reno 10.0 1 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Sack1 Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_sack -superclass TestSuite
Test/ecn_nodrop_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_sack
        set guide_      "Sack with ECN."
        $self next pktTraceFile
}
Test/ecn_nodrop_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_sack -superclass TestSuite
Test/ecn_twoecn_sack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_sack
        set guide_      "Sack, two marked packets in one window."
        $self next pktTraceFile
}
Test/ecn_twoecn_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN preceded by packet loss.
# Do one with ECN followed by packet loss?
Class Test/ecn_drop_sack -superclass TestSuite
Test/ecn_drop_sack instproc init {} {
        $self instvar net_ test_ guide_
#        Queue/RED set setbit_ true
        set net_        net2-lossy
        set test_       ecn_drop_sack
        set guide_      "Sack, ECN preceded by packet loss."
        $self next pktTraceFile
} 
Test/ecn_drop_sack instproc run {} {
        global quiet
        $self instvar ns_ guide_
        puts "Guide: $guide_"
        $self ecnsetup Sack1 3.0
        $self drop_pkt 195
        $ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop1_sack -superclass TestSuite
Test/ecn_drop1_sack instproc init {} {
        $self instvar net_ test_ guide_
#        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_sack
        set guide_      "Sack, ECN preceded by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop1_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0
	$self drop_pkt 185
	$ns_ run
}

# Packet loss only.
Class Test/ecn_noecn_sack -superclass TestSuite
Test/ecn_noecn_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 1000
	Queue/RED set maxthresh_ 1000
        set net_	net2-lossy
        set test_	ecn_noecn_sack
	Test/ecn_noecn_sack instproc run {} [Test/ecn_drop_sack info instbody run ]
        set guide_      "Sack, packet loss only, no ECN."
        $self next pktTraceFile
}

# Multiple dup acks 
Class Test/ecn_bursty_sack -superclass TestSuite
Test/ecn_bursty_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
        set test_	ecn_bursty_sack
        set guide_      "Sack, multiple dup acks."
        $self next pktTraceFile
}
Test/ecn_bursty_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"

	$self ecnsetup Sack1 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 189
	$lossmodel set burstlen_ 15
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn_sack -superclass TestSuite
Test/ecn_burstyEcn_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_burstyEcn_sack
	Test/ecn_burstyEcn_sack instproc run {} [Test/ecn_bursty_sack info instbody run ]   
        set guide_      "Sack, multiple dup acks and ECN."
        $self next pktTraceFile
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn1_sack -superclass TestSuite
Test/ecn_burstyEcn1_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_burstyEcn1_sack
        set guide_      "Sack, multiple dup acks and ECN."
        $self next pktTraceFile
}
Test/ecn_burstyEcn1_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"

	$self ecnsetup Sack1 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 189
	$lossmodel set burstlen_ 17
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks without bugFix_
Class Test/ecn_noBugfix_sack -superclass TestSuite
Test/ecn_noBugfix_sack instproc init {} {
        $self instvar net_ test_ guide_
	Queue/RED set thresh_ 100 
	Queue/RED set maxthresh_ 100
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ false
        set test_	ecn_noBugfix_sack
	Test/ecn_noBugfix_sack instproc run {} [Test/ecn_bursty_sack info instbody run ]

        set guide_      "Sack, multiple dup acks, no bugFix."
        $self next pktTraceFile
}

# ECN followed by timeout.
Class Test/ecn_timeout_sack -superclass TestSuite
Test/ecn_timeout_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout_sack
        set guide_      "Sack, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 1
	$self drop_pkts {242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268} 

	$ns_ run
}

# ECN followed by a timeout, followed by an ECN representing a
# new instance of congestion.
Class Test/ecn_timeout1_sack -superclass TestSuite
Test/ecn_timeout1_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout1_sack
        set guide_      "Sack, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout1_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 1
	$self drop_pkts {245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265} 
	$self second_tcp Tahoe 1.0
	$ns_ run
}

# ECN and packet drops.
Class Test/ecn_fourdrops_sack -superclass TestSuite
Test/ecn_fourdrops_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_fourdrops_sack
        set guide_      "Reno, ECN with four packet drops."
        $self next pktTraceFile
}
Test/ecn_fourdrops_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 1
	$self drop_pkts {182 184 207 208} 

	$ns_ run
}

# Packet drops with a window of one packet.
Class Test/ecn_smallwin_sack -superclass TestSuite
Test/ecn_smallwin_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwin_sack
        set guide_      "Sack, packet drops with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwin_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 6.0 1
	$self drop_pkts {4 8 9 10 11 12 13 100 115 121 124 126 127 128}

	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_sack -superclass TestSuite
Test/ecn_smallwinEcn_sack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_sack
        set guide_      "Sack, ECN with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Sack1 10.0 1
	$self drop_pkts {4 7 9 10 11 12 13 14 15 120 135 143 148 150 151 152} 
	$errmodel1 set markecn_ true
	$ns_ run
}

Class Test/ecn_smallwin1Ecn_sack -superclass TestSuite
Test/ecn_smallwin1Ecn_sack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net3-lossy
        set test_	ecn_smallwin1Ecn_sack
        set guide_      "Sack, ECN with small windows."
        $self next pktTraceFile
}
Test/ecn_smallwin1Ecn_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1 errmodel2
        puts "Guide: $guide_"
	$self ecnsetup Sack1 10.0 1
	$self drop_pkts {4 8 9 11 12 13 120 135 143 148 150 153} 
	$errmodel1 set markecn_ true
	$self drop_pkts2 {6}
	$errmodel2 set markecn_ false
	$ns_ run
}

# Packet drops for the second packet.
Class Test/ecn_secondpkt_sack -superclass TestSuite
Test/ecn_secondpkt_sack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpkt_sack
        set guide_      "Sack, with the second packet dropped."
        $self next pktTraceFile
}
Test/ecn_secondpkt_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 2.0 1
	$self drop_pkts {1 3} 

	$ns_ run
}

# ECN for the second packet.
Class Test/ecn_secondpktEcn_sack -superclass TestSuite
Test/ecn_secondpktEcn_sack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpktEcn_sack
        set guide_      "Sack, with the second packet marked."
        $self next pktTraceFile
}
Test/ecn_secondpktEcn_sack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Sack1 2.0 1
	$self drop_pkts {1 3} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Delayed Ack Sack1 Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_sack_delack -superclass TestSuite
Test/ecn_nodrop_sack_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_sack_delack
        set guide_      "Sack, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_nodrop_sack_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 0 1 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_sack_delack -superclass TestSuite
Test/ecn_twoecn_sack_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_sack_delack
        set guide_      "Sack, ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_twoecn_sack_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 0 1 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_sack_delack -superclass TestSuite
Test/ecn_drop_sack_delack instproc init {} {
        $self instvar net_ test_ guide_
#        Queue/RED set setbit_ true
        set net_        net2-lossy
        set test_       ecn_drop_sack_delack
        set guide_      "Sack, packet loss followed by ECN, delayed acks."
        $self next pktTraceFile
} 
Test/ecn_drop_sack_delack instproc run {} {
        global quiet
        $self instvar ns_ guide_
        puts "Guide: $guide_"
        $self ecnsetup Sack1 3.0 0 1
        $self drop_pkt 195
        $ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop1_sack_delack -superclass TestSuite
Test/ecn_drop1_sack_delack instproc init {} {
        $self instvar net_ test_ guide_
#        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_sack_delack
        set guide_      "Sack, packet loss followed by ECN, delayed acks."
        $self next pktTraceFile
}
Test/ecn_drop1_sack_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Sack1 3.0 0 1
	$self drop_pkt 185
	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_sack_delack -superclass TestSuite
Test/ecn_smallwinEcn_sack_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_sack_delack
        set guide_      "Sack, delayed ack, ECN with small windows."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_sack_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Sack1 10.0 1 1
	$self drop_pkts {4 7 9 11 12 13 14 15 120 135 143 148 150 151 152} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Newreno Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_newreno -superclass TestSuite
Test/ecn_nodrop_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_newreno
        set guide_      "NewReno with ECN."
        $self next pktTraceFile
}
Test/ecn_nodrop_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
	$self ecnsetup Newreno 3.0 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
Class Test/ecn_twoecn_newreno -superclass TestSuite
Test/ecn_twoecn_newreno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_newreno
        set guide_      "NewReno, two marked packets in one window."
        $self next pktTraceFile
}
Test/ecn_twoecn_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_newreno -superclass TestSuite
Test/ecn_drop_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_newreno
        set guide_      "NewReno, ECN preceded by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
	$self ecnsetup Newreno 3.0
	$self drop_pkt 195
	$ns_ run
}

# ECN preceded by packet loss.
Class Test/ecn_drop1_newreno -superclass TestSuite
Test/ecn_drop1_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_newreno
        set guide_      "NewReno, ECN preceded by packet loss."
        $self next pktTraceFile
}
Test/ecn_drop1_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0
	$self drop_pkt 185
	$ns_ run
}

# Packet loss only.
Class Test/ecn_noecn_newreno -superclass TestSuite
Test/ecn_noecn_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 1000
	Queue/RED set maxthresh_ 1000
        set net_	net2-lossy
        set test_	ecn_noecn_newreno
	Test/ecn_noecn_newreno instproc run {} [Test/ecn_drop_newreno info instbody run ]
        set guide_      "NewReno, packet loss only, no ECN."
        $self next pktTraceFile
}

# Multiple dup acks 
Class Test/ecn_bursty_newreno -superclass TestSuite
Test/ecn_bursty_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
	Queue/RED set thresh_ 100
	Queue/RED set maxthresh_ 100
        set net_	net2-lossy
        set test_	ecn_bursty_newreno
        set guide_      "NewReno, multiple dup acks."
        $self next pktTraceFile
}
Test/ecn_bursty_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"

	$self ecnsetup Newreno 3.0
        set lossmodel [$self setloss]
        $lossmodel set offset_ 189
	$lossmodel set burstlen_ 15
        $lossmodel set period_ 10000
	$ns_ run
}

# Multiple dup acks following ECN
Class Test/ecn_burstyEcn_newreno -superclass TestSuite
Test/ecn_burstyEcn_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_burstyEcn_newreno
	Test/ecn_burstyEcn_newreno instproc run {} [Test/ecn_bursty_newreno info instbody run ]   
        set guide_      "NewReno, multiple dup acks and ECN."
        $self next pktTraceFile
}

# ECN followed by timeout.
# Nope.
Class Test/ecn_timeout_newreno -superclass TestSuite
Test/ecn_timeout_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout_newreno
        set guide_      "NewReno, multiple dup acks"
        $self next pktTraceFile
}
Test/ecn_timeout_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 1
	$self drop_pkts {242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268} 

	$ns_ run
}

# ECN followed by a timeout, followed by an ECN representing a
# new instance of congestion.
Class Test/ecn_timeout1_newreno -superclass TestSuite
Test/ecn_timeout1_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_timeout1_newreno
        set guide_      "NewReno, drops and ECN followed by a timeout."
        $self next pktTraceFile
}
Test/ecn_timeout1_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 1
	$self drop_pkts {245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265} 
	$self second_tcp Tahoe 1.0
	$ns_ run
}

# Packet drops with a window of one packet.
Class Test/ecn_smallwin_newreno -superclass TestSuite
Test/ecn_smallwin_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwin_newreno
        set guide_      "NewReno, packet drops with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwin_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 6.0 1
	$self drop_pkts {4 8 9 10 11 12 13 100 115 121 124 126 127 128}

	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_newreno -superclass TestSuite
Test/ecn_smallwinEcn_newreno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_newreno
        set guide_      "NewReno, ECN with a window of one."
        $self next pktTraceFile
}
Test/ecn_smallwinEcn_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Newreno 10.0 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 

	$errmodel1 set markecn_ true
	$ns_ run
}

# Packet drops for the second packet.
Class Test/ecn_secondpkt_newreno -superclass TestSuite
Test/ecn_secondpkt_newreno instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpkt_newreno
        set guide_      "NewReno, with the second packet dropped."
        $self next pktTraceFile
}
Test/ecn_secondpkt_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 2.0 1
	$self drop_pkts {1 3} 

	$ns_ run
}

# ECN for the second packet.
Class Test/ecn_secondpktEcn_newreno -superclass TestSuite
Test/ecn_secondpktEcn_newreno instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_secondpktEcn_newreno
        set guide_      "NewReno, with the second packet marked."
        $self next pktTraceFile
}
Test/ecn_secondpktEcn_newreno instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Newreno 2.0 1
	$self drop_pkts {1 3} 
	$errmodel1 set markecn_ true
	$ns_ run
}

#######################################################################
# Delayed Ack Newreno Tests #
#######################################################################

# Plain ECN
Class Test/ecn_nodrop_newreno_delack -superclass TestSuite
Test/ecn_nodrop_newreno_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_nodrop_newreno_delack
        set guide_      "NewReno, plain ECN, delayed ACKs."
        $self next pktTraceFile
}
Test/ecn_nodrop_newreno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 0 1 
	$self drop_pkt 10000
	$ns_ run
}

# Two ECNs close together
# Nope.
Class Test/ecn_twoecn_newreno_delack -superclass TestSuite
Test/ecn_twoecn_newreno_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_twoecn_newreno_delack
        set guide_	"NewReno, delayed acks."

        $self next pktTraceFile
}
Test/ecn_twoecn_newreno_delack instproc run {} {
        global quiet guide_
	$self instvar ns_ guide_ lossmodel
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 0 1 
	$self drop_pkt 195
	$lossmodel set markecn_ true
	$ns_ run
}

# ECN followed by packet loss.
Class Test/ecn_drop_newreno_delack -superclass TestSuite
Test/ecn_drop_newreno_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop_newreno_delack
        set guide_  "ECN followed by packet loss."

        $self next pktTraceFile
}
Test/ecn_drop_newreno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 0 1
	$self drop_pkt 195
	$ns_ run
}

# ECN preceded by packet loss.
Class Test/ecn_drop1_newreno_delack -superclass TestSuite
Test/ecn_drop1_newreno_delack instproc init {} {
        $self instvar net_ test_ guide_
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_drop1_newreno_delack
	set guide_ "ECN preceded by packet loss"
        $self next pktTraceFile
}
Test/ecn_drop1_newreno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_
        puts "Guide: $guide_"
	$self ecnsetup Newreno 3.0 0 1
	$self drop_pkt 185
	$ns_ run
}

# ECN with a window of one packet.
Class Test/ecn_smallwinEcn_newreno_delack -superclass TestSuite
Test/ecn_smallwinEcn_newreno_delack instproc init {} {
        $self instvar net_ test_ guide_ 
        Queue/RED set setbit_ true
        set net_	net2-lossy
        set test_	ecn_smallwinEcn_newreno_delack
	set guide_ "ECN with a window of one packet."

        $self next pktTraceFile
}
Test/ecn_smallwinEcn_newreno_delack instproc run {} {
        global quiet
	$self instvar ns_ guide_ errmodel1
        puts "Guide: $guide_"
	$self ecnsetup Newreno 10.0 1 1
	$self drop_pkts {4 8 9 11 12 13 14 15 120 135 143 148 150 151 152 153} 

	$errmodel1 set markecn_ true
	$ns_ run
}

#################################################################

#################################################################
# 
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 20ms bottleneck.
# Queue-limit on bottleneck is 25 packets.
#    
Class Topology/net6 -superclass Topology
Topology/net6 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    Queue/RED set setbit_ true
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 20ms RED
    $ns queue-limit $node_(r1) $node_(k1) 25
    $ns queue-limit $node_(k1) $node_(r1) 25
} 

# This test shows two TCPs when one is ECN-capable and the other 
# is not.

Class Test/ecn1 -superclass TestSuite
Test/ecn1 instproc init {} {
        $self instvar net_ test_ guide_
        set net_        net6
        set test_       ecn1_(one_with_ecn1,_one_without)
	set guide_ 	"Two TCPs, one with ECN and one without."
        $self next pktTraceFile
}
Test/ecn1 instproc run {} {
        global quiet
        $self instvar ns_ guide_ node_ guide_ testName_
        puts "Guide: $guide_"
        $self setTopo

        # Set up TCP connection 
        set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
        $tcp1 set window_ 30
        $tcp1 set ecn_ 1
        set ftp1 [$tcp1 attach-app FTP]
        $ns_ at 0.0 "$ftp1 start"

        # Set up TCP connection
        set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
        $tcp2 set window_ 20
        $tcp2 set ecn_ 0
        set ftp2 [$tcp2 attach-app FTP]
        $ns_ at 3.0 "$ftp2 start"

        $self tcpDump $tcp1 5.0
        $self tcpDump $tcp2 5.0 

        #$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_" 

        $ns_ run
}

TestSuite runTest
