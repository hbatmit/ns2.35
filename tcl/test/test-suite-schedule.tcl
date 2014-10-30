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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-schedule.tcl,v 1.18 2006/01/24 23:00:07 sallyfloyd Exp $
#
# To view a list of available tests to run with this script:
# ns test-suite-schedule.tcl
#

set quiet false

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21

Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
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

Class Topology

Topology instproc node? num {
	 $self instvar node_
	 return $node_($num)
}

Topology instproc makeNet1 { ns scheduler { delay2 20ms } } {
	$self instvar node_
    	set node_(s1) [$ns node]
    	set node_(s2) [$ns node]
    	set node_(r1) [$ns node]
    	set node_(k1) [$ns node]

        $ns duplex-link $node_(s1) $node_(r1) 8Mb 2ms DropTail
        $ns duplex-link $node_(s2) $node_(r1) 8Mb $delay2 DropTail
        $ns duplex-link $node_(r1) $node_(k1) 800Kb 2ms $scheduler
        $ns queue-limit $node_(r1) $node_(k1) 25 
        $ns queue-limit $node_(k1) $node_(r1) 25 
	if {[$class info instprocs config] != ""} {
	 $self config $ns
 	}

}

Class Topology/netSFQ -superclass Topology
Topology/netSFQ instproc init ns {
	$self instvar node_
	$self makeNet1 $ns SFQ 20ms
}

Class Topology/netFQ -superclass Topology
Topology/netFQ instproc init ns {
	$self instvar node_
	$self makeNet1 $ns FQ 20ms
}

Class Topology/netDRR -superclass Topology
Topology/netDRR instproc init ns {
	$self instvar node_
	$self makeNet1 $ns DRR 10ms
}

Class Topology/netRED -superclass Topology
Topology/netRED instproc init ns {
	$self instvar node_
	$self makeNet1 $ns RED 10ms
}

Class Topology/netDT -superclass Topology
Topology/netDT instproc init ns {
	$self instvar node_
	$self makeNet1 $ns DropTail 10ms
}

TestSuite instproc finish file {
	global quiet PERL
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
          $PERL ../../bin/raw2xg -s 0.01 -m 90 -t $file > temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
	}
        ## now use default graphing tool to make a data file
	## if so desired
        exit 0
}

TestSuite instproc printtimers { tcp time} {
	global quiet
	if {$quiet == "false"} {
        	puts "time: $time sRTT(in ticks): [$tcp set srtt_]/8 RTTvar(in ticks): [$tcp set rttvar_]/4 backoff: [$tcp set backoff_]"
	}
}

TestSuite instproc printtimersAll { tcp time interval } {
        $self instvar dump_inst_ ns_
        if ![info exists dump_inst_($tcp)] {
                set dump_inst_($tcp) 1
                $ns_ at $time "$self printtimersAll $tcp $time $interval"
                return
        }
	set newTime [expr [$ns_ now] + $interval]
	$ns_ at $time "$self printtimers $tcp $time"
        $ns_ at $newTime "$self printtimersAll $tcp $newTime $interval"
}

TestSuite instproc runDetailed {} {
	global quiet
	$self instvar ns_ node_ testName_

	# Set up TCP connection
	set tcp1 [$ns_ create-connection TCP $node_(s1) TCPSink $node_(k1) 0]
	$tcp1 set window_ 20
	set ftp1 [$tcp1 attach-app FTP]
	$ns_ at 0.1 "$ftp1 start"
	

	# Set up TCP connection
	set tcp2 [$ns_ create-connection TCP $node_(s2) TCPSink $node_(k1) 1]
	$tcp2 set window_ 20
	set ftp2 [$tcp2 attach-app FTP]
	$ns_ at 0.4 "$ftp2 start"

        $self tcpDump $tcp1 5.0
        $self tcpDump $tcp2 5.0

        #$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
	$ns_ at 10.0 "$self cleanupAll $testName_"
        $ns_ run
}

Class Test/fifo-droptail -superclass TestSuite
Test/fifo-droptail instproc init {} {
        $self instvar net_ test_
        set net_        netDT
        set test_       fifo-droptail
        $self next
}
Test/fifo-droptail instproc run {} {
	Agent/TCP set overhead_ 0.01
	$self setTopo
	$self runDetailed
}

Class Test/fifo-red -superclass TestSuite
Test/fifo-red instproc init {} {
        $self instvar net_ test_
        set net_        netRED
        set test_       fifo-red
        $self next
}
Test/fifo-red instproc run {} {
	$self setTopo
	$self runDetailed
}

Class Test/sfq -superclass TestSuite
Test/sfq instproc init {} {
        $self instvar net_ test_
        set net_        netSFQ
        set test_       sfq
        $self next
}
Test/sfq instproc run {} {
	$self setTopo
	$self runDetailed
}

Class Test/fq -superclass TestSuite
Test/fq instproc init {} {
        $self instvar net_ test_ 
        set net_        netFQ
        set test_       fq
        $self next
}
Test/fq instproc run {} {
	$self setTopo
	$self runDetailed
}

Class Test/fq_small_queue -superclass TestSuite
Test/fq_small_queue instproc init {} {
        $self instvar net_ test_
        set net_        netFQ
        set test_       fq_small_queue
        $self next
}
Test/fq_small_queue instproc run {} {
	$self instvar node_
	$self setTopo 
	Queue set limit_ 12
	$self runDetailed
}

Class Test/drr -superclass TestSuite
Test/drr instproc init {} {
        $self instvar net_ test_
        set net_        netDRR
        set test_       drr
        $self next
}
Test/drr instproc run {} {
	$self setTopo
	$self runDetailed
}


TestSuite runTest

#######################################################
