#
# Copyright (c) 2000  International Computer Science Institute
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
#      This product includes software developed by ACIRI, the AT&T
#      Center for Internet Research at ICSI (the International Computer
#      Science Institute).
# 4. Neither the name of ACIRI nor of ICSI may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
catch "cd $dir"
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Queue/RED set bytes_ false              
# default changed on 10/11/2004.
Queue/RED set queue_in_bytes_ false
# default changed on 10/11/2004.
Queue/RED set q_weight_ 0.002
Queue/RED set thresh_ 5 
Queue/RED set maxthresh_ 15
# The RED parameter defaults are being changed for automatic configuration.
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.
Agent/TCP set windowInit_ 1
# The default is being changed to 2.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Queue/RED set gentle_ true
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.
Agent/TCP set timerfix_ false
# The default is being changed to true.

source ../ex/red-pd/monitoring.tcl
set target_rtt_ 0.040
set testIdent_ 0
set verbosity_ -1
set listMode_ multi
set unresponsive_test_ 1
source ../ex/red-pd/helper.tcl

set flowfile fairflow.tr; # file where flow data is written
set flowgraphfile fairflow.xgr; # file given to graph tool 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -a -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	if { [info exists tchan_] && $quiet == "false" } {
		$self plotQueue $testName_
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

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_ redpdflowmon_ redpdq_ redpdlink_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns simplex-link $node_(r1) $node_(r2) 0.5Mb 20ms RED/PD 
    $ns simplex-link $node_(r2) $node_(r1) 0.5Mb 20ms DropTail
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail

    set redpdlink_ [$ns link $node_(r1) $node_(r2)]
    set redpdq_ [$redpdlink_ queue]
    set redpdflowmon_ [$redpdq_ makeflowmon $redpdlink_]
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
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

TestSuite instproc monitorFlow {fid prob} {
    $self instvar topo_ 
    $topo_ instvar redpdflowmon_ redpdq_

    foreach flow [$redpdflowmon_ flows] {
	if {[$flow set flowid_] == $fid} {
	    #monitor the flow with probability $prob
	    $redpdq_ monitor-flow $flow $prob 
	}
    }	
}

TestSuite instproc unmonitorFlow {fid} {
    $self instvar topo_ 
    $topo_ instvar redpdflowmon_ redpdq_

    foreach flow [$redpdflowmon_ flows] {
	if {[$flow set flowid_] == $fid} {
	    #unmonitor the flow
	    $redpdq_ unmonitor-flow $flow 
	}
    }	
}

TestSuite instproc unresponsiveFlow {fid penalty} {
    $self instvar topo_ 
    $topo_ instvar redpdflowmon_ redpdq_

    foreach flow [$redpdflowmon_ flows] {
	if {[$flow set flowid_] == $fid} {
	    #set the flow unresponsive 
	    $redpdq_ unresponsive-flow $flow 
	    #set unresponsive penalty imposed on an unresponsive flow
	    $redpdq_ set unresponsive_penalty_ $penalty
	}
    }	
}

#
# Start monitoring tcp1 at time 10.0 with probability 0.1,
# and unmonitor it at 20.0
#
Class Test/simple -superclass TestSuite
Test/simple instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ simple
    $self next pktTraceFile
}
Test/simple instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $topo_ instvar redpdflowmon_ redpdq_


    set stoptime 30.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 2]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    $ns_ at 10.0 "$self monitorFlow 1 0.1"
    $ns_ at 20.0 "$self unmonitorFlow 1"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# Start monitoring udp at time 10.0 with probability 0.1, 
# declare it unresponsive at 20.0 and set unresponsive_penalty_ 3.0
#
Class Test/unresponsive -superclass TestSuite
Test/unresponsive instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ unresponsive
    $self next pktTraceFile
}
Test/unresponsive instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $topo_ instvar redpdflowmon_ redpdq_

    set stoptime 30.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 2]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    set udp [$ns_ create-connection UDP $node_(s1) Null $node_(s4) 3]
    set cbr [$udp attach-app Traffic/CBR]
    $cbr set rate_ 0.5Mb
    $ns_ at 1.0 "$cbr start"
    
    $ns_ at 10.0 "$self monitorFlow 3 0.1"
    $ns_ at 20.0 "$self unresponsiveFlow 3 3"

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# drop from all flows at a fixed configured probability
#
Class Test/frp -superclass TestSuite
Test/frp instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ frp
    $self next pktTraceFile
}
Test/frp instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $topo_ instvar redpdflowmon_ redpdq_

    set stoptime 30.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 2]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    #set the constant dropping probability
    $redpdq_ set P_testFRp_ 0.1

    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# one complete test using the identification engine
#
Class Test/complete -superclass TestSuite
Test/complete instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ complete
    $self next pktTraceFile
}
Test/complete instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $topo_ instvar redpdflowmon_ redpdq_ redpdlink_

    set stoptime 30.0
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 1]
    $tcp1 set window_ 50
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 2]
    $tcp2 set window_ 50
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"

    set udp [$ns_ create-connection UDP $node_(s1) Null $node_(s4) 3]
    set cbr [$udp attach-app Traffic/CBR]
    $cbr set rate_ 0.5Mb
    $ns_ at 1.0 "$cbr start"

    set redpdsim [new REDPDSim $ns_ $redpdq_ $redpdflowmon_ $redpdlink_ 0 true]
    
    $self tcpDump $tcp1 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

TestSuite instproc printall { fmon time packetsize} {
	set packets [$fmon set pdepartures_]
	set linkBps [ expr 500000/8 ]
	set utilization [expr ($packets*$packetsize)/($time*$linkBps)]
	puts "link utilization [format %.3f $utilization]"
}

#
# one complete test using the identification engine, with CBR flows only
#
Class Test/cbrs -superclass TestSuite
Test/cbrs instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs
    Queue/RED/PD set noidle_ false
    $self next pktTraceFile
}
Test/cbrs instproc run {} {
    $self instvar ns_ node_ testName_ net_ topo_
    $self setTopo
    $topo_ instvar redpdflowmon_ redpdq_ redpdlink_
    set packetsize_ 200
    Application/Traffic/CBR set random_ 0
    Application/Traffic/CBR set packetSize_ $packetsize_

    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
    $ns_ attach-fmon $slink $fmon

    set stoptime 30.0
    #set stoptime 5.0
    set udp1 [$ns_ create-connection UDP $node_(s1) Null $node_(s3) 1]
    set cbr1 [$udp1 attach-app Traffic/CBR]
    $cbr1 set rate_ 0.1Mb
    $cbr1 set random_ 0.005

    set udp2 [$ns_ create-connection UDP $node_(s2) Null $node_(s3) 2]
    set cbr2 [$udp2 attach-app Traffic/CBR]
    $cbr2 set rate_ 0.1Mb
    $cbr2 set random_ 0.005

    $self enable_tracequeue $ns_
    $ns_ at 0.2 "$cbr1 start"
    $ns_ at 0.1 "$cbr2 start"

    set udp [$ns_ create-connection UDP $node_(s1) Null $node_(s4) 3]
    set cbr [$udp attach-app Traffic/CBR]
    $cbr set rate_ 0.5Mb
    $cbr set random_ 0.001
    $ns_ at 0.0 "$cbr start"

    set redpdsim [new REDPDSim $ns_ $redpdq_ $redpdflowmon_ $redpdlink_ 0 true]
    
    $self timeDump 5.0
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self printall $fmon $stoptime $packetsize_"
    $ns_ at $stoptime "$self cleanupAll $testName_"
    $ns_ run
}

#
# one complete test using the identification engine, with CBR flows only,
# with noidle set to true, so that we don't drop from unresponsive flows
# when it would result in the link going idle
#
Class Test/cbrs-noidle -superclass TestSuite
Test/cbrs-noidle instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ cbrs-noidle
    Queue/RED/PD set noidle_ true
    Test/cbrs-noidle instproc run {} [Test/cbrs info instbody run ]
    $self next pktTraceFile
}
TestSuite runTest
