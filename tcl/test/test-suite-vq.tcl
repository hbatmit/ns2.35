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
# To run all tests: test-all-vq

source misc_simple.tcl
remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP  ; # hdrs reqd for validation test
 
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
Agent/TCP set singledup_ 0
Agent/TCP set overhead_ 0.001
Queue/Vq set buflim_ 0.25
# The default is being changed to 1

Queue/Vq set queue_in_bytes_ false
Queue/Vq set gamma_ 0.895
# The defaults for queue_in_bytes_ and gamma_ are being changed.

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
	set vqq [[$ns link $node_(r1) $node_(r2)] queue]
	set tchan_ [open all.q w]
	$vqq trace curq_
	$vqq attach $tchan_
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 20ms Vq
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
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 10ms Vq
    $ns duplex-link $node_(r2) $node_(r1) 1.5Mb 10ms Vq
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
	# Plot the queue size and average queue size, for Vq gateways.
	#
	set awkCode {
		{
			if ($1 == "Q" && NF>2) {
				print $2, $3 >> "temp.q";
				set end $2
			}
		}
	}
	set f [open temp.queue w]
	puts $f "TitleText: $file"
	puts $f "Device: Postscript"

	if { [info exists tchan_] } {
		close $tchan_
	}
	exec rm -f temp.q
	exec touch temp.q

	exec awk $awkCode all.q

	puts $f \"queue
	exec cat temp.q >@ $f  
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

Class Test/vq1 -superclass TestSuite
Test/vq1 instproc init {} {
    $self instvar net_ test_
    set net_ net2 
    set test_ vq1
    Queue/Vq set buflim_ 0.25
    $self next pktTraceFile
}
Test/vq1 instproc run {} {
    $self instvar ns_ node_ testName_ net_
    $self setTopo

    set vqq [[$ns_ link $node_(r1) $node_(r2)] queue]

   	$vqq set markpkts_ false 
   	$vqq set queue_in_bytes_ false 

    set stoptime 10.0

   	$vqq set markpkts_ false 
   	$vqq set queue_in_bytes_ false 

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 15

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
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

Class Test/ecn -superclass TestSuite
Test/ecn instproc init {} {
    $self instvar net_ test_
    Queue/Vq set setbit_ true
    Agent/TCP set old_ecn_ 1
    set net_	net2
    set test_	ecn
    $self next pktTraceFile
}
Test/ecn instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo 

    set stoptime 10.0
    set vqq [[$ns_ link $node_(r1) $node_(r2)] queue]
		
   	$vqq set markpkts_ true 
   	$vqq set queue_in_bytes_ false 

    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 15
    $tcp1 set ecn_ 1

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
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

Class Test/vq2 -superclass TestSuite
Test/vq2 instproc init {} {
    $self instvar net_ test_
    set net_	net3
    set test_	vq2
    $self next pktTraceFile
}
Test/vq2 instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set stoptime 10.0
    set vqq [[$ns_ link $node_(r1) $node_(r2)] queue]

   	$vqq set markpkts_ false 
   	$vqq set queue_in_bytes_ false 
	
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 100 

    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s3) 1]
    $tcp2 set window_ 100

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
Class Test/vq_twoway -superclass TestSuite
Test/vq_twoway instproc init {} {
    $self instvar net_ test_
    set net_	net3
    set test_	vq_twoway
    $self next pktTraceFile
}
Test/vq_twoway instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set stoptime 10.0
    set vqq [[$ns_ link $node_(r1) $node_(r2)] queue]

   	$vqq set markpkts_ false 
   	$vqq set queue_in_bytes_ false 
	
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 100 
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    $tcp2 set window_ 100 
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    set tcp3 [$ns_ create-connection TCP/Sack1 $node_(s3) TCPSink/Sack1 $node_(s1) 2]
    $tcp3 set window_ 100 
    set tcp4 [$ns_ create-connection TCP/Sack1 $node_(s4) TCPSink/Sack1 $node_(s2) 3]
    $tcp4 set window_ 100 
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
Class Test/vq_twowaybytes -superclass TestSuite
Test/vq_twowaybytes instproc init {} {
    $self instvar net_ test_
    set net_	net2
    set test_	vq_twowaybytes
    $self next pktTraceFile
}
Test/vq_twowaybytes instproc run {} {
    $self instvar ns_ node_ testName_
    $self setTopo

    set stoptime 10.0
    set vqq [[$ns_ link $node_(r1) $node_(r2)] queue]
   	$vqq set markpkts_ false 
   	$vqq set queue_in_bytes_ false 
		
    set tcp1 [$ns_ create-connection TCP/Sack1 $node_(s1) TCPSink/Sack1 $node_(s3) 0]
    $tcp1 set window_ 15
    set tcp2 [$ns_ create-connection TCP/Sack1 $node_(s2) TCPSink/Sack1 $node_(s4) 1]
    $tcp2 set window_ 15
    set ftp1 [$tcp1 attach-app FTP]
    set ftp2 [$tcp2 attach-app FTP]

    set tcp3 [$ns_ create-connection TCP/Sack1 $node_(s3) TCPSink/Sack1 $node_(s1) 2]
    $tcp3 set window_ 15
    set tcp4 [$ns_ create-connection TCP/Sack1 $node_(s4) TCPSink/Sack1 $node_(s2) 3]
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

        exec sort -n -k2 -o $flowfile $flowfile
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
	set tcp [$ns_ create-connection TCP/Sack1 $source TCPSink/Sack1 $dest $fid]
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



TestSuite runTest
