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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-sack-full.tcl,v 1.22 2006/01/24 23:00:07 sallyfloyd Exp $
#

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for TCP

Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
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

Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.

set style raw2xg
# set style tcpf2xgr

Trace set show_tcphdr_ 1 ;	# needed for plotting ACK numbers

TestSuite instproc finish file {
        global quiet PERL style
	set wrap 90
	set wrap1 [expr $wrap * 512 + 40]
	set space 512
	set scale 0.01
	if { $style == "tcpf2xgr" } {
		set scale [expr $scale * (1.0 / 576.0)]
		set space 1
		set outtype "xgraph"
		set tfile "xxx.tr"
		if { $quiet != "true" } {
			global TCLSH PERL
			exec rm -f $tfile
			exec $PERL ../../bin/getrc -b -s 2 -d 3 all.tr > $tfile
			puts "Wrote file $tfile (output from getrc -b -s 2 -d 3)..."
			exec ../../bin/tcpf2xgr -n $space -s $scale -m $wrap1 $TCLSH $tfile $outtype $file &
			puts "copying $tfile to temp.rands"
			exec cp $tfile temp.rands ; # needed for compare script
			# tcpf2xgr runs xgraph for us
		}
 	} elseif { $style == "raw2xg" } {
		exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
		   $PERL ../../bin/raw2xg -c -n $space -s 0.01 -m $wrap1 -t $file \
		   > temp.rands
	        exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	           $PERL ../../bin/raw2xg -a -c -f -n $space -s 0.01 -m $wrap1 -t $file > temp1.rands
	
		if {$quiet == "false"} {
			# exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
		# to plot acks:
			exec xgraph -bb -tk -nl -m -x time -y packets temp.rands temp1.rands &
		}
        }
        ## now use default graphing tool to make a data file
        ## if so desired
        exit 0
}

Class Topology

Topology instproc node? num {
    $self instvar node_
    return $node_($num)
}

# 
# Links1 uses 8Mb, 5ms feeders, and a 800Kb 100ms bottleneck.
# Queue-limit on bottleneck is 6 packets. 
# 
Class Topology/net0 -superclass Topology
Topology/net0 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next
    $ns duplex-link $node_(s1) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 8Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 800Kb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 6
    $ns queue-limit $node_(k1) $node_(r1) 6
}

# 
# Links1 uses 10Mb, 5ms feeders, and a 1.5Mb 100ms bottleneck.
# Queue-limit on bottleneck is 23 packets.
# 

Class Topology/net1 -superclass Topology
Topology/net1 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(k1) [$ns node]

    $self next 
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 5ms DropTail
    $ns duplex-link $node_(r1) $node_(k1) 1.5Mb 100ms DropTail
    $ns queue-limit $node_(r1) $node_(k1) 19
    $ns queue-limit $node_(k1) $node_(r1) 19
}


Class Topology/net2 -superclass Topology
Topology/net2 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]
    set node_(r1) [$ns node]
    set node_(r2) [$ns node]
    set node_(s3) [$ns node]
    set node_(s4) [$ns node]

    $self next 
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms RED
    $ns queue-limit $node_(r1) $node_(r2) 25
    $ns queue-limit $node_(r2) $node_(r1) 25
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail
}

TestSuite instproc makeproto { aopen popen win fid apptype appstart } {
	$self instvar ns_ node_

	set tcp1 [new Agent/TCP/FullTcp/Sack]
	set sink [new Agent/TCP/FullTcp/Sack]
	$ns_ attach-agent $node_($aopen) $tcp1
	$ns_ attach-agent $node_($popen) $sink
	$tcp1 set window_ $win
	$sink set window_ $win
	$tcp1 set fid_ $fid
	$sink set fid_ $fid
	$ns_ connect $tcp1 $sink
	# set up TCP-level connections
	$sink listen ; # will figure out who its peer is

	if { $apptype != "none" } {
		set apphandle [$tcp1 attach-app $apptype]
		$ns_ at $appstart "$apphandle start"
	}
	$self tcpDump $tcp1 1.0
	return $tcp1
}


# single packet drop
Class Test/sack1 -superclass TestSuite
Test/sack1 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1
    $self next pktTraceFile
}
Test/sack1 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    $self makeproto s1 k1 14 0 FTP 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1z -superclass TestSuite
Test/sack1z instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1z
    $self next pktTraceFile
}
Test/sack1z instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4
    $self makeproto s1 k1 14 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

# three packet drops
Class Test/sack1a -superclass TestSuite
Test/sack1a instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1a
    $self next pktTraceFile
}
Test/sack1a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    $self makeproto s1 k1 20 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

# three packet drops
Class Test/sack1aa -superclass TestSuite
Test/sack1aa instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1aa
    $self next pktTraceFile
}
Test/sack1aa instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4

    $self makeproto s1 k1 20 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1b -superclass TestSuite
Test/sack1b instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1b
    $self next pktTraceFile
}
Test/sack1b instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $self makeproto s1 k1 26 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack1c -superclass TestSuite
Test/sack1c instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack1c
    $self next pktTraceFile
}
Test/sack1c instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $self makeproto s1 k1 27 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 5.0 $testName_]
    $ns_ at 5.0 "$self cleanupAll $testName_"

    $ns_ run
}

Class Test/sack3 -superclass TestSuite
Test/sack3 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sack3
    $self next pktTraceFile
}
Test/sack3 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(k1) 8
    $ns_ queue-limit $node_(k1) $node_(r1) 8

    [$self makeproto s1 k1 100 0 FTP 0.95] set bugFix_ false
    [$self makeproto s2 k1 16 1 FTP 0.5] set bugFix_ false

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
    $ns_ at 4.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack5 -superclass TestSuite
Test/sack5 instproc init {} {
    $self instvar net_ test_
    set net_	net1
    set test_	sack5
    $self next pktTraceFile
}
Test/sack5 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set window_ 50
    Agent/TCP set bugFix_ false
    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    [$self makeproto s1 k1 50 0 FTP 1.0] set bugFix_ false
    set tcp2 [$self makeproto s2 k1 50 1 none -1]
    $tcp2 set bugFix_ false

    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 1.75 "$ftp2 produce 100"

    $ns_ at 6.0 "$self cleanupAll $testName_"
    $ns_ run
}

Class Test/sack5a -superclass TestSuite
Test/sack5a instproc init {} {
    $self instvar net_ test_
    set net_	net1
    set test_	sack5a
    $self next pktTraceFile
}
Test/sack5a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    Agent/TCP set maxburst_ 4
    $ns_ delay $node_(s1) $node_(r1) 3ms
    $ns_ delay $node_(r1) $node_(s1) 3ms

    [$self makeproto s1 k1 50 0 FTP 1.0] set bugFix_ false
    set tcp2 [$self makeproto s2 k1 50 1 none -1]
    $tcp2 set bugFix_ false

    set ftp2 [$tcp2 attach-app FTP]
    $ns_ at 1.75 "$ftp2 produce 100"

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 6.0 $testName_]
    $ns_ at 6.0 "$self cleanupAll $testName_"
    $ns_ run
}

# shows a long recovery from sack.
Class Test/sackB2 -superclass TestSuite
Test/sackB2 instproc init {} {
    $self instvar net_ test_
    set net_	net0
    set test_	sackB2
    $self next pktTraceFile
}
Test/sackB2 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(k1) 9

    $self makeproto s1 k1 50 0 FTP 1.0
    $self makeproto s2 k1 20 1 FTP 1.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 8.0 $testName_]
    $ns_ at 8.0 "$self cleanupAll $testName_"
    $ns_ run
}

# two packets dropped
Class Test/sackB4 -superclass TestSuite
Test/sackB4 instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	sackB4
    $self next pktTraceFile
}
Test/sackB4 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(r2) 29

    $self makeproto s1 r2 40 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
    $ns_ at 2.0 "$self cleanupAll $testName_"
    $ns_ run
}

# two packets dropped
Class Test/sackB4a -superclass TestSuite
Test/sackB4a instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	sackB4a
    $self next pktTraceFile
}
Test/sackB4a instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo
    $ns_ queue-limit $node_(r1) $node_(r2) 29
    Agent/TCP set maxburst_ 4

    $self makeproto s1 r2 40 0 FTP 0.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace 2.0 $testName_]
    $ns_ at 2.0 "$self cleanupAll $testName_"
    $ns_ run
}

# delayed ack not implemented yet
#Class Test/delayedSack -superclass TestSuite
#Test/delayedSack instproc init {} {
#    $self instvar net_ test_
#    set net_    net0
#    set test_	delayedSack
#    $self next pktTraceFile
#}
#Test/delayedSack instproc run {} {
#     $self instvar ns_ node_ testName_
#     $self setTopo
#     set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#     $tcp1 set window_ 50
# 
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
#     set ftp1 [$tcp1 attach-app FTP]
#     $ns_ at 1.0 "$ftp1 start"
# 
#     $self tcpDump $tcp1 1.0
# 
#     # trace only the bottleneck link
#     #$self traceQueues $node_(r1) [$self openTrace 4.0 $testName_]
#     $ns_ run
# }

## segregation
#Class Test/phaseSack -superclass TestSuite
#Test/phaseSack instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack
#    $self next pktTraceFile
#}
#Test/phaseSack instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 3ms
#    $ns_ delay $node_(r1) $node_(s2) 3ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#
#    $self tcpDump $tcp1 5.0
#
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}
#
## random overhead, but segregation remains 
#Class Test/phaseSack2 -superclass TestSuite
#Test/phaseSack2 instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack2
#    $self next pktTraceFile
#}
#Test/phaseSack2 instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 3ms
#    $ns_ delay $node_(r1) $node_(s2) 3ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#    $tcp1 set overhead_ 0.01
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#    $tcp2 set overhead_ 0.01
#    
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#    
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#    
#    $self tcpDump $tcp1 5.0
#    
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}
#
## no segregation, because of random overhead
#Class Test/phaseSack3 -superclass TestSuite
#Test/phaseSack3 instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	phaseSack3
#    $self next pktTraceFile
#}
#Test/phaseSack3 instproc run {} {
#    $self instvar ns_ node_ testName_
#    $self setTopo
#
#    $ns_ delay $node_(s2) $node_(r1) 9.5ms
#    $ns_ delay $node_(r1) $node_(s2) 9.5ms
#    $ns_ queue-limit $node_(r1) $node_(k1) 16
#    $ns_ queue-limit $node_(k1) $node_(r1) 100
#
#    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#    $tcp1 set window_ 32
#    $tcp1 set overhead_ 0.01
#
#    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#    $tcp2 set window_ 32
#    $tcp2 set overhead_ 0.01
#
#    set ftp1 [$tcp1 attach-app FTP]
#    set ftp2 [$tcp2 attach-app FTP]
#
#    $ns_ at 5.0 "$ftp1 start"
#    $ns_ at 1.0 "$ftp2 start"
#
#    $self tcpDump $tcp1 5.0
#
#    # trace only the bottleneck link
#    #$self traceQueues $node_(r1) [$self openTrace 25.0 $testName_]
#    $ns_ run
#}

#Class Test/timersSack -superclass TestSuite
#Test/timersSack instproc init {} {
#    $self instvar net_ test_
#    set net_	net0
#    set test_	timersSack
#    $self next pktTraceFile
#}
#Test/timersSack instproc run {} {
#     $self instvar ns_ node_ testName_
#     $self setTopo
#     $ns_ queue-limit $node_(r1) $node_(k1) 2
#     $ns_ queue-limit $node_(k1) $node_(r1) 100
# 
#     set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(k1) 0]
#     $tcp1 set window_ 4
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp1 dst-port]] set interval 100ms
# 
#     set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(k1) 1]
#     $tcp2 set window_ 4
#     # lookup up the sink and set it's delay interval
#     [$node_(k1) agent [$tcp2 dst-port]] set interval 100ms
# 
#     set ftp1 [$tcp1 attach-app FTP]
#     set ftp2 [$tcp2 attach-app FTP]
# 
#     $ns_ at 1.0 "$ftp1 start"
#     $ns_ at 1.3225 "$ftp2 start"
# 
#     $self tcpDump $tcp1 5.0
# 
#     # trace only the bottleneck link
#     #$self traceQueues $node_(r1) [$self openTrace 10.0 $testName_]
#     $ns_ run
# }

TestSuite runTest

