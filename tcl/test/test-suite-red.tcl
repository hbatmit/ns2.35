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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-red.tcl,v 1.64 2007/09/25 05:30:57 sallyfloyd Exp $
#
# This test suite reproduces most of the tests from the following note:
# Floyd, S., 
# Ns Simulator Tests for Random Early Detection (RED), October 1996.
# URL ftp://ftp.ee.lbl.gov/papers/redsims.ps.Z.
#
# To run all tests: test-all-red

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
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
Queue/RED set use_mark_p_ false
# The default is being changed to true.

catch "cd $dir"
#Agent/TCP set oldCode_ true
Agent/TCP set minrto_ 0
# The default is being changed to minrto_ 1
Agent/TCP set syn_ false
Agent/TCP set delay_growth_ false
# In preparation for changing the default values for syn_ and delay_growth_.
Agent/TCP set SetCWRonRetransmit_ true
# Changing the default value.

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
 
    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up

    # force identical behavior to ns-1.
    # the recommended value for linterm is now 10
    # and is placed in the default file (3/31/97)
    [[$ns link $node_(r1) $node_(r2)] queue] set linterm_ 50
    [[$ns link $node_(r2) $node_(r1)] queue] set linterm_ 50
}   

Class Topology/net3 -superclass Topology
Topology/net3 instproc init ns {
    $self instvar node_
    set node_(s1) [$ns node]
    set node_(s2) [$ns node]    
    set node_(r1) [$ns node]    
    set node_(r2) [$ns node]    
    set node_(s3) [$ns node]    
    set node_(s4) [$ns node]    

    $self next 

    $ns duplex-link $node_(s1) $node_(r1) 10Mb 0ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 1ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 10ms RED
    $ns duplex-link $node_(r2) $node_(r1) 1.5Mb 10ms RED
    $ns queue-limit $node_(r1) $node_(r2) 100
    $ns queue-limit $node_(r2) $node_(r1) 100
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 2ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 3ms DropTail
 
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

TestSuite instproc mainSim {TCPStyle {window 15} } {
    $self instvar ns_ node_ testName_ 
    set stoptime 10.0
    
    if {$TCPStyle=="Reno"} {
	set sourceType TCP/Reno;  
	set sinkType TCPSink;
    } elseif {$TCPStyle=="Sack1"} {
	set sourceType TCP/Sack1;  
	set sinkType TCPSink/Sack1;
    }
    set tcp1 [$ns_ create-connection $sourceType $node_(s1) $sinkType $node_(s3) 0]
    set tcp2 [$ns_ create-connection $sourceType $node_(s2) $sinkType $node_(s3) 1]
    $tcp1 set window_ $window
    $tcp2 set window_ $window

    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 3.0 "$ftp2 start"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

Class Test/red1 -superclass TestSuite
Test/red1 instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net2 
    set test_ red1
    set guide_ "RED, without ECN."
    $self next
}
Test/red1 instproc run {} {
    $self instvar ns_ node_ testName_ net_ guide_
    puts "Guide: $guide_"
    $self setTopo
    $self mainSim Reno
    $ns_ run
}

Class Test/ecn -superclass TestSuite
Test/ecn instproc init {} {
    $self instvar net_ test_ guide_
    Queue/RED set setbit_ true
    Agent/TCP set old_ecn_ 1
    set net_	net2
    set test_	ecn
    set guide_ "RED with ECN."
    $self next
}
Test/ecn instproc run {} {
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

# "Red2" changes some of the RED gateway parameters.
# This should give worse performance than "red1".
Class Test/red2 -superclass TestSuite
Test/red2 instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	red2
    set guide_ "RED, without ECN, with different parameters."
    $self next
}
Test/red2 instproc run {} {
    $self instvar ns_ node_ testName_ guide_
    puts "Guide: $guide_"
    $self setTopo

    set stoptime 10.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set thresh_ 5
    $redq set maxthresh_ 10
    $redq set q_weight_ 0.003
	
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s3) 1]
    $tcp2 set window_ 15

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

# The queue is measured in "packets".
Class Test/red_twoway -superclass TestSuite
Test/red_twoway instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	red_twoway
    set guide_ "RED, two-way traffic, queue measured in packets."
    $self next
}
Test/red_twoway instproc run {} {
    $self instvar ns_ node_ testName_ guide_
    puts "Guide: $guide_"
    $self setTopo

    set stoptime 10.0
	
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s4) 1]
    $tcp2 set window_ 15
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    set tcp3 [$ns_ create-connection TCP/Reno $node_(s3) TCPSink $node_(s1) 2]
    $tcp3 set window_ 15
    set tcp4 [$ns_ create-connection TCP/Reno $node_(s4) TCPSink $node_(s2) 3]
    $tcp4 set window_ 15
    set ftp3 [$tcp3 attach-app FTP]
    set telnet1 [$tcp4 attach-app Telnet] ; $telnet1 set interval_ 0

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 2.0 "$ftp2 start"
    $ns_ at 3.5 "$ftp3 start"
    $ns_ at 1.0 "$telnet1 start"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"

    $ns_ run
}

# The queue is measured in "bytes".
Class Test/red_twowaybytes -superclass TestSuite
Test/red_twowaybytes instproc init {} {
    $self instvar net_ test_ guide_
    set net_	net2
    set test_	red_twowaybytes
    set guide_ "RED, two-way traffic, queue measured in bytes."
    Queue/RED set ns1_compat_ true
    $self next
}
Test/red_twowaybytes instproc run {} {
    $self instvar ns_ node_ testName_ guide_
    puts "Guide: $guide_"
    $self setTopo

    set stoptime 10.0
    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]
    $redq set bytes_ true
    $redq set queue_in_bytes_ true
		
    set tcp1 [$ns_ create-connection TCP/Reno $node_(s1) TCPSink $node_(s3) 0]
    $tcp1 set window_ 15
    set tcp2 [$ns_ create-connection TCP/Reno $node_(s2) TCPSink $node_(s4) 1]
    $tcp2 set window_ 15
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    set tcp3 [$ns_ create-connection TCP/Reno $node_(s3) TCPSink $node_(s1) 2]
    $tcp3 set window_ 15
    set tcp4 [$ns_ create-connection TCP/Reno $node_(s4) TCPSink $node_(s2) 3]
    $tcp4 set window_ 15
    set ftp3 [$tcp3 attach-app FTP]
    set telnet1 [$tcp4 attach-app Telnet] ; $telnet1 set interval_ 0

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 2.0 "$ftp2 start"
    $ns_ at 3.5 "$ftp3 start"
    $ns_ at 1.0 "$telnet1 start"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"

    $ns_ run
}

#
#######################################################################

TestSuite instproc create_flowstats {} {

	global flowfile flowchan
	$self instvar ns_ node_ r1fm_

	set r1fm_ [$ns_ makeflowmon Fid]
	set flowchan [open $flowfile w]
	$r1fm_ attach $flowchan
	$ns_ attach-fmon [$ns_ link $node_(r1) $node_(r2)] $r1fm_ 1
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [pkts]
#	(verified compatible for ns2 - kfall, 10/30/97)
TestSuite instproc unforcedmakeawk { } {
        set awkCode {
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2;
			if ($13 > 0 && $14 > 0) {
			    print 100.0 * $9/$13, 100.0 * $10 / $14
			}
			prev = $2;
                } else if ($13 > 0 && $14 > 0) {
                        print 100.0 * $9 / $13, 100.0 * $10 / $14
		}
            }
        }
        return $awkCode
}

#
# awk code used to produce:
#       x axis: # arrivals for this flow+category / # total arrivals [bytes]
#       y axis: # drops for this flow+category / # drops this category [bytes]
#	(modified for compatibility with ns2 flowmon - kfall, 10/30/97)
TestSuite instproc forcedmakeawk { } {
        set awkCode {
            BEGIN { print "\"flow 0" }
            {
                if ($2 != prev) {
                        print " "; print "\"flow " $2;
			if ($13 > 0 && ($17 - $15) > 0) {
				print 100.0 * $9/$13, 100.0 * ($19 - $11) / ($17 - $15);
			}
			prev = $2;
                } else if ($13 > 0 && ($17 - $15) > 0) {
                        print 100.0 * $9 / $13, 100.0 * ($19 - $11) / ($17 - $15)
		}
            }
        }
        return $awkCode
}

#
# awk code used to produce:
#      x axis: # arrivals for this flow+category / # total arrivals [bytes]
#      y axis: # drops for this flow / # drops [pkts and bytes combined]
TestSuite instproc allmakeawk { } {
    set awkCode {
        BEGIN {prev=-1; tot_bytes=0; tot_packets=0; forced_total=0; unforced_total=0}
        {
            if ($5 != prev) {
                print " "; print "\"flow ",$5;
                prev = $5
            }
            tot_bytes = $19-$11;
            forced_total= $16-$14;
            tot_packets = $10;
            tot_arrivals = $9;
            unforced_total = $14;
            if (unforced_total + forced_total > 0) {
                if ($14 > 0) {
                    frac_packets = tot_packets/$14;
                }
                else { frac_packets = 0;}
                if ($17-$15 > 0) {
                    frac_bytes = tot_bytes/($17-$15);
                }
                else {frac_bytes = 0;} 
                if ($13 > 0) {
                    frac_arrivals = tot_arrivals/$13;
                }
                else {frac_arrivals = 0;}
                if (frac_packets + frac_bytes > 0) {
                    unforced_total_part = frac_packets * unforced_total / ( unforced_total + forced_total)
                    forced_total_part = frac_bytes * forced_total / ( unforced_total + forced_total)
                    print 100.0 * frac_arrivals, 100.0 * ( unforced_total_part +forced_total_part)
                }
            }
        }
    }
    return $awkCode
}

TestSuite instproc create_flow_graph { graphtitle graphfile } {
        global flowfile quiet
	$self instvar awkprocedure_

        if {$quiet == "false"} {
		puts "awkprocedure: $awkprocedure_"
	}

        set tmpfile1 /tmp/fg1[pid]
        set tmpfile2 /tmp/fg2[pid]

        exec rm -f $graphfile
        set outdesc [open $graphfile w]
        #
        # this next part is xgraph specific
        #
        puts $outdesc "TitleText: $graphtitle"
        puts $outdesc "Device: Postscript"

        exec rm -f $tmpfile1 $tmpfile2
	if {$quiet == "false"} {
        	puts "writing flow xgraph data to $graphfile..."
	}

        exec sort -n -k2 -k1 -o $flowfile $flowfile
        exec awk [$self $awkprocedure_] $flowfile >@ $outdesc
        close $outdesc
}

TestSuite instproc finish_flows testname {
	global flowgraphfile flowfile flowchan quiet
	$self instvar r1fm_
	$r1fm_ dump
	close $flowchan
	$self create_flow_graph $testname $flowgraphfile
	if {$quiet == "false"} {
		puts "running xgraph..."
	}
	exec cp $flowgraphfile temp.rands
	if {$quiet == "false"} {
		exec xgraph -bb -tk -nl -m -lx 0,100 -ly 0,100 -x "% of data bytes" -y "% of discards" $flowgraphfile &
	}
	exit 0
}


TestSuite instproc new_tcp { startTime source dest window fid verbose size } {
	$self instvar ns_
	set tcp [$ns_ create-connection TCP/Reno $source TCPSink $dest $fid]
	$tcp set window_ $window
	if {$size > 0}  {$tcp set packetSize_ $size }
	set ftp [$tcp attach-app FTP]
	$ns_ at $startTime "$ftp start"
	if {$verbose == "1"} {

	  $self tcpDumpAll $tcp 20.0 $fid 
	}
}

TestSuite instproc new_cbr { startTime source dest pktSize interval fid } {

	$self instvar ns_
    set s_agent [new Agent/UDP]	
    set d_agent [new Agent/LossMonitor]
    $s_agent set fid_ $fid 
    $d_agent set fid_ $fid 
    set cbr [new Application/Traffic/CBR]
    $cbr attach-agent $s_agent
    $ns_ attach-agent $source $s_agent
    $ns_ attach-agent $dest $d_agent
    $ns_ connect $s_agent $d_agent

    if {$pktSize > 0} {
	$cbr set packetSize_ $pktSize
    }
    $cbr set rate_ [expr 8 * $pktSize / $interval]
    $ns_ at $startTime "$cbr start"
}

TestSuite instproc dumpflows interval {
    $self instvar dumpflows_inst_ ns_ r1fm_
    $self instvar awkprocedure_ dump_pthresh_
    global flowchan

    if ![info exists dumpflows_inst_] {
        set dumpflows_inst_ 1
        $ns_ at 0.0 "$self dumpflows $interval"
        return  
    }
    if { $awkprocedure_ == "unforcedmakeawk" } {
	set pcnt [$r1fm_ set epdrops_]
    } elseif { $awkprocedure_ == "forcedmakeawk" } {
	set pcnt [expr [$r1fm_ set pdrops_] - [$r1fm_ set epdrops_]]
    } elseif { $awkprocedure_ == "allmakeawk" } {
	set pcnt [$r1fm_ set pdrops_]
    } else {
	puts stderr "unknown handling of flow dumps!"
	exit 1
    }
    if { $pcnt >= $dump_pthresh_ } {
	    $r1fm_ dump
	    flush $flowchan
	    foreach f [$r1fm_ flows] {
		$f reset
	    }
	    $r1fm_ reset
	    set interval 2.0
    } else {
	    set interval 1.0
    }
    $ns_ at [expr [$ns_ now] + $interval] "$self dumpflows $interval"
}   

TestSuite instproc droptest { stoptime } {
	$self instvar ns_ node_ testName_ r1fm_ awkprocedure_

	set forwq [[$ns_ link $node_(r1) $node_(r2)] queue]
	set revq [[$ns_ link $node_(r2) $node_(r1)] queue]

	$forwq set mean_pktsize_ 1000
	$revq set mean_pktsize_ 1000
	$forwq set linterm_ 10
	$revq set linterm_ 10
	$forwq set limit_ 100
	$revq set limit_ 100

	$self create_flowstats 
	$self dumpflows 10.0

	$forwq set bytes_ true
	$forwq set queue_in_bytes_ true
	$forwq set wait_ false

        $self new_tcp 1.0 $node_(s1) $node_(s3) 100 1 1 1000
	$self new_tcp 1.2 $node_(s2) $node_(s4) 100 2 1 50
	$self new_cbr 1.4 $node_(s1) $node_(s4) 190 0.003 3

	$ns_ at $stoptime "$self finish_flows $testName_"

	$ns_ run
}


Class Test/flows_unforced -superclass TestSuite
Test/flows_unforced instproc init {} {
    $self instvar net_ test_ guide_
    set net_    net2   
    set test_   flows_unforced
    set guide_ "RED, sending rate vs. packet drop rate, unforced packet drops."
    Queue/RED set gentle_ false
    $self next noTraceFiles; # zero here means don't product all.tr
}   

Test/flows_unforced instproc run {} {

	$self instvar ns_ node_ testName_ r1fm_ awkprocedure_ guide_
        puts "Guide: $guide_"
	$self instvar dump_pthresh_
	$self setTopo
        set stoptime 500.0
	set testName_ test_flows_unforced
	set awkprocedure_ unforcedmakeawk
	set dump_pthresh_ 100

	$self droptest $stoptime

}

Class Test/flows_forced -superclass TestSuite
Test/flows_forced instproc init {} {
    $self instvar net_ test_ guide_
    set net_    net2   
    set test_   flows_forced
    set guide_ "RED, sending rate vs. packet drop rate, forced packet drops."
    Queue/RED set gentle_ false
    $self next noTraceFiles; # zero here means don't product all.tr
}   

Test/flows_forced instproc run {} {

	$self instvar ns_ node_ testName_ r1fm_ awkprocedure_ guide_
        puts "Guide: $guide_"
	$self instvar dump_pthresh_
	$self setTopo
 
        set stoptime 500.0
	set testName_ test_flows_forced
	set awkprocedure_ forcedmakeawk
	set dump_pthresh_ 100

	$self droptest $stoptime
}

Class Test/flows_combined -superclass TestSuite
Test/flows_combined instproc init {} {
    $self instvar net_ test_ guide_
    set net_    net2   
    set test_   flows_combined
    set guide_ "RED, sending rate vs. packet drop rate, all packet drops."
    Queue/RED set gentle_ false
    $self next noTraceFiles; # zero here means don't product all.tr
}   

Test/flows_combined instproc run {} {

	$self instvar ns_ node_ testName_ r1fm_ awkprocedure_ guide_
	$self instvar dump_pthresh_
        puts "Guide: $guide_"
	$self setTopo
 
        set stoptime 500.0
	set testName_ test_flows_combined
	set awkprocedure_ allmakeawk
	set dump_pthresh_ 100

	$self droptest $stoptime
}

#--------------------------------------------------------------

TestSuite instproc printall { fmon } {
        puts "total_drops [$fmon set pdrops_] total_packets [$fmon set pdepartures_]"
}

Class Test/ungentle -superclass TestSuite
Test/ungentle instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ ungentle
    set guide_ "RED, not gentle."
    Queue/RED set gentle_ false
    $self next
}
Test/ungentle instproc run {} {
    $self instvar ns_ node_ testName_ net_ guide_
    puts "Guide: $guide_"
    $self setTopo
    Agent/TCP set packetSize_ 1500
    Agent/TCP set window_ 50
    Queue/RED set bytes_ true
    Agent/TCP set timerfix_ false
    # The default is being changed to true.

    set stoptime 40.0
    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
   #$ns_ attach-fmon $slink $fmon
    $ns_ attach-fmon $slink $fmon 1
    
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set packetSize_ 1000
    set tcp3 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 2]
    set tcp4 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 3]
    $tcp4 set packetSize_ 512
    set tcp5 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 4]
    set tcp6 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 5]
    set tcp7 [$ns_ create-connection TCP/Sack1 $node_(s4) TCPSink/Sack1 $node_(s2) 6]
    set tcp8 [$ns_ create-connection TCP/Sack1 $node_(s3) TCPSink/Sack1 $node_(s1) 7]
    $tcp8 set packetSize_ 2000


    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]
    set ftp3 [$tcp3 attach-app FTP]
    set ftp4 [$tcp4 attach-app FTP]
    set ftp5 [$tcp5 attach-app FTP]
    set ftp6 [$tcp6 attach-app FTP]
    set ftp7 [$tcp7 attach-app FTP]
    set ftp8 [$tcp8 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 1.0 "$ftp2 start"
    $ns_ at 5.0 "$ftp3 start"
    $ns_ at 6.0 "$ftp4 start"
    $ns_ at 9.0 "$ftp5 start"
    $ns_ at 10.0 "$ftp6 start"
    $ns_ at 13.0 "$ftp7 start"
    $ns_ at 14.0 "$ftp8 start"
    $ns_ at $stoptime "$self printall $fmon"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"

    $ns_ run
}

Class Test/gentle -superclass TestSuite
Test/gentle instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ gentle
    set guide_ "RED, gentle."
    Queue/RED set gentle_ true
    Test/gentle instproc run {} [Test/ungentle info instbody run ]
    $self next
}

Class Test/gentleEcn -superclass TestSuite
Test/gentleEcn instproc init {} {
    $self instvar net_ test_ guide_
    Queue/RED set setbit_ true
    Agent/TCP set ecn_ 1
    set net_ net3 
    set test_ gentleEcn
    set guide_ "RED, gentle, with ECN."
    Queue/RED set gentle_ true
    Test/gentleEcn instproc run {} [Test/ungentle info instbody run ]
    $self next
}

Class Test/gentleEcn1 -superclass TestSuite
Test/gentleEcn1 instproc init {} {
    $self instvar net_ test_ guide_
    Queue/RED set setbit_ true
    Agent/TCP set ecn_ 1
    set net_ net3 
    set test_ gentleEcn1
    set guide_ "RED, gentle, with ECN, with mark_p_ set to 0.1."
    Queue/RED set gentle_ true
    Queue/RED set mark_p_ 0.1
    Queue/RED set use_mark_p_ true
    Test/gentleEcn1 instproc run {} [Test/ungentle info instbody run ]
    $self next
}

Class Test/ungentleBadParams -superclass TestSuite
Test/ungentleBadParams instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ ungentleBadParams
    set guide_ "RED, not gentle, bad RED parameters."
    Queue/RED set gentle_ false
    Queue/RED set linterm_ 50
    Queue/RED set maxthresh_ 10
    Test/ungentleBadParams instproc run {} [Test/ungentle info instbody run ]
    $self next
}

Class Test/gentleBadParams -superclass TestSuite
Test/gentleBadParams instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ gentleBadParams
    Queue/RED set gentle_ true
    set guide_ "RED, gentle, bad RED parameters."
    Queue/RED set linterm_ 50
    Queue/RED set maxthresh_ 10
    Test/gentleBadParams instproc run {} [Test/ungentle info instbody run ]
    $self next
}

Class Test/q_weight -superclass TestSuite
Test/q_weight instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net2 
    set test_ q_weight
    set guide_ "RED, q_weight set to 0.002."
    $self next
}
Test/q_weight instproc run {} {
    $self instvar ns_ node_ testName_ net_ guide_
    puts "Guide: $guide_"
    $self setTopo
    $self mainSim Sack1
    $ns_ run
}

Class Test/q_weight_auto -superclass TestSuite
Test/q_weight_auto instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net2 
    set test_ q_weight_auto
    Queue/RED set q_weight_ 0.0
    set guide_ "RED, q_weight and maxthresh_ set automatically."
    Queue/RED set maxthresh_ 0
    Test/q_weight_auto instproc run {} [Test/q_weight info instbody run ]
    $self next
}

# Class Test/q_weight1 -superclass TestSuite
# Test/q_weight1 instproc init {} {
#     $self instvar net_ test_ guide_
#     set net_ net2 
#     set test_ q_weight
#     $self next
# }
# Test/q_weight1 instproc run {} {
#     $self instvar ns_ node_ testName_ net_
#     $self setTopo
#     $ns_ at 0.0 "$ns_ bandwidth $node_(r1) $node_(r2) 100Mb duplex"
#     $self mainSim Sack1 100
#     $ns_ run
# }
# 
# Class Test/q_weight1_auto -superclass TestSuite
# Test/q_weight1_auto instproc init {} {
#     $self instvar net_ test_ guide_
#     set net_ net2 
#     set test_ q_weight1_auto
#     Queue/RED set q_weight_ 0.0
#     Queue/RED set maxthresh_ 0
#     Test/q_weight1_auto instproc run {} [Test/q_weight1 info instbody run ]
#     $self next
# }

##
## Packets are marked instead of dropped if the average queue is
## less than maxthresh.
##
Class Test/congested -superclass TestSuite
Test/congested instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ congested
    set guide_ "RED, not gentle, ECN."
    Queue/RED set use_mark_p_ false
    $self next
}
Test/congested instproc run {} {
    $self instvar ns_ node_ testName_ net_ guide_
    puts "Guide: $guide_"
    Agent/TCP set packetSize_ 1500
    Agent/TCP set window_ 75
    Agent/TCP set ecn_ 1
    Queue/RED set bytes_ true
    Queue/RED set gentle_ false
    Queue/RED set setbit_ true
    $self setTopo
    # The default is being changed to true.

    set stoptime 5.0
    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
   #$ns_ attach-fmon $slink $fmon
    $ns_ attach-fmon $slink $fmon 1
    
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]

    set ftp1 [$tcp1 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at $stoptime "$self printall $fmon"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"

    $ns_ run
}

##
## Packets are marked instead of dropped if the buffer is not full
##
Class Test/congested_mark_p -superclass TestSuite
Test/congested_mark_p instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ congested_mark_p
    set guide_ "RED, not gentle, ECN.  Packets not dropped unless buffer full."
    Queue/RED set mark_p_ 2.0
    Queue/RED set use_mark_p_ true
    Test/congested_mark_p instproc run {} [Test/congested info instbody run ]
    $self next
}

##
## Packets are marked instead of dropped if the drop probability
## is less than one.
##
Class Test/congested1_mark_p -superclass TestSuite
Test/congested1_mark_p instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ congested1_mark_p
    set guide_ "RED, gentle, ECN.  Packets sometimes dropped instead of marked."
    Queue/RED set mark_p_ 1.0
    Queue/RED set use_mark_p_ true
    $self next
}
Test/congested1_mark_p instproc run {} {
    $self instvar ns_ node_ testName_ net_ guide_
    puts "Guide: $guide_"
    Agent/TCP set packetSize_ 1500
    Agent/TCP set window_ 1000
    Agent/TCP set ecn_ 1
    Queue/RED set bytes_ true
    Queue/RED set gentle_ true
    Queue/RED set setbit_ true
    $self setTopo
    # The default is being changed to true.

    set stoptime 5.0
    set slink [$ns_ link $node_(r1) $node_(r2)]; # link to collect stats on
    set fmon [$ns_ makeflowmon Fid]
   #$ns_ attach-fmon $slink $fmon
    $ns_ attach-fmon $slink $fmon 1
    
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    set ftp1 [$tcp1 attach-app FTP]
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 0]
    set ftp2 [$tcp2 attach-app FTP]

    $self enable_tracequeue $ns_
    $ns_ at 0.0 "$ftp1 start"
    $ns_ at 0.2 "$ftp2 start"
    $ns_ at $stoptime "$self printall $fmon"

    $self tcpDump $tcp1 5.0

    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"

    $ns_ run
}

##
## Packets are marked instead of dropped if the buffer is not full.
##
Class Test/congested2_mark_p -superclass TestSuite
Test/congested2_mark_p instproc init {} {
    $self instvar net_ test_ guide_
    set net_ net3 
    set test_ congested2_mark_p
    set guide_ "RED, gentle, ECN.  Packets not dropped unless buffer full."
    Queue/RED set mark_p_ 2.0
    Queue/RED set use_mark_p_ true
    Test/congested2_mark_p instproc run {} [Test/congested1_mark_p info instbody run ]
    $self next
}
TestSuite runTest
