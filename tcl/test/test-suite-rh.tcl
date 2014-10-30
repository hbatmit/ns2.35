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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-rh.tcl,v 1.11 2006/01/24 23:00:07 sallyfloyd Exp $
#
# To run all tests: test-all-ecn

set dir [pwd]
catch "cd tcl/test"
source misc_simple.tcl
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
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
catch "cd $dir"

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

    set node_(a1) [$ns node]
    set node_(a2) [$ns node]
    set node_(a3) [$ns node]
    set node_(a4) [$ns node]
    set node_(a5) [$ns node]
    set node_(a6) [$ns node]
    set node_(b1) [$ns node]
    set node_(b2) [$ns node]
    set node_(b3) [$ns node]
    set node_(b4) [$ns node]
    set node_(b5) [$ns node]
    set node_(b6) [$ns node]
}

Topology instproc createlinks ns {  
    $self instvar node_
    $ns duplex-link $node_(s1) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(s2) $node_(r1) 10Mb 15ms DropTail
    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 30ms RED
#    $ns duplex-link $node_(r1) $node_(r2) 1.5Mb 30ms DropTail
    $ns queue-limit $node_(r1) $node_(r2) 40
    $ns queue-limit $node_(r2) $node_(r1) 40
    $ns duplex-link $node_(s3) $node_(r2) 10Mb 4ms DropTail
    $ns duplex-link $node_(s4) $node_(r2) 10Mb 5ms DropTail

# Now create a mess of links for lots of conns 
    $ns duplex-link $node_(a1) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(b1) $node_(r2) 10Mb 23ms DropTail
    $ns duplex-link $node_(a2) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(b2) $node_(r2) 10Mb 22ms DropTail
    $ns duplex-link $node_(a3) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(b3) $node_(r2) 10Mb 33ms DropTail
    $ns duplex-link $node_(a4) $node_(r1) 10Mb 4ms DropTail
    $ns duplex-link $node_(b4) $node_(r2) 10Mb 15ms DropTail
    $ns duplex-link $node_(a5) $node_(r1) 10Mb 3ms DropTail
    $ns duplex-link $node_(b5) $node_(r2) 10Mb 12ms DropTail
    $ns duplex-link $node_(a6) $node_(r1) 10Mb 2ms DropTail
    $ns duplex-link $node_(b6) $node_(r2) 10Mb 27ms DropTail

    $ns duplex-link-op $node_(s1) $node_(r1) orient right-down
    $ns duplex-link-op $node_(s2) $node_(r1) orient right-up
    $ns duplex-link-op $node_(r1) $node_(r2) orient right
    $ns duplex-link-op $node_(r1) $node_(r2) queuePos 0
    $ns duplex-link-op $node_(r2) $node_(r1) queuePos 0
    $ns duplex-link-op $node_(s3) $node_(r2) orient left-down
    $ns duplex-link-op $node_(s4) $node_(r2) orient left-up
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
 
    $self instvar lossylink_ lossylink2_

    set lossylink_ [$ns link $node_(r1) $node_(r2)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass

    set lossylink2_ [$ns link $node_(s1) $node_(r1)]
    set em [new ErrorModule Fid]
    set errmodel [new ErrorModel/Periodic]
    $errmodel unit pkt
    $lossylink2_ errormodule $em
    $em insert $errmodel
    $em bind $errmodel 0
    $em default pass
} 

TestSuite instproc finish file {
	global quiet PERL
	$self instvar ns_ tchan_ testName_ cwnd_chan_ xlimits_ fig_file_ stimes_
        exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
	   $PERL ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp.rands
	exec $PERL ../../bin/getrc -s 3 -d 2 all.tr | \
	  $PERL ../../bin/raw2xg -a -e -s 0.01 -m 90 -t $file > temp1.rands
	if {$quiet == "false"} {
#		exec xgraph -bb -tk -nl -m -x time -y packets temp.rands &
# The line below plots both data and ack packets.
#        	exec xgraph -bb -tk -nl -m -x time -y packets temp.rands \
#                     temp1.rands &
# Here we plot again for the paper with extra limits:
	        exec echo Disposition: To File > temp_fig.rands
	        exec echo FileOrDev: $fig_file_ >> temp_fig.rands
	        exec cat temp.rands >> temp_fig.rands
		exec xgraph -bb -tk -nl -m -x time -y packets -lx $xlimits_ temp_fig.rands &
	}
        ## now use default graphing tool to make a data file
        ## if so desired

	#  Keep the numerical results for later use:
#	set ofile_ [open data.out a]
#	set ecn_count [exec jgrep E-N all.tr | grep r | grep " 2 3 tcp" |  wc -l ]
#	set drop_count [exec jgrep d all.tr |  wc -l ]
#	set awkcode { {print $2} }
#	set end_time [exec tail -1 all.tr | awk $awkcode]
#	set bw [expr 1800000.0*8.0/$end_time/15000]
#	puts $ofile_ "$testName_ $ecn_count $drop_count $end_time $bw $stimes_"
#	close $ofile_

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
	$topo_ instvar lossylink2_
        set errmodule [$lossylink2_ errormodule]
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

TestSuite instproc enable_tracecwnd { ns tcp } {
        $self instvar cwnd_chan_ 
	if { ! [info exists cwnd_chan_] } then {
	    set cwnd_chan_ [open all.cwnd w]
	}
        $tcp trace cwnd_
        $tcp attach $cwnd_chan_
}

TestSuite instproc plot_cwnd {} {
        global quiet
        $self instvar cwnd_chan_
        set awkCode {
              {
	      if ($6 == "cwnd_") {
	      	print $1, $7 >> "temp.cwnd";
	      } }
        } 
        set f [open cwnd.xgr w]
        puts $f "TitleText: cwnd"
        puts $f "Device: Postscript"

        if { [info exists cwnd_chan_] } {
                close $cwnd_chan_
        }
        exec rm -f temp.cwnd 
        exec touch temp.cwnd

        exec awk $awkCode all.cwnd

        puts $f \"cwnd
        exec cat temp.cwnd >@ $f
        close $f
        if {$quiet == "false"} {
                exec xgraph -M -bb -tk -x time -y cwnd cwnd.xgr &
        }
}

TestSuite instproc netsetup { {stoptime 3.0} {ecnmode false} } {
    $self instvar ns_ node_ testName_ net_
    $self setTopo
##
##  Agent/TCP set maxburst_ 4
##
    set delay 30ms
    $ns_ delay $node_(r1) $node_(r2) $delay
    $ns_ delay $node_(r2) $node_(r1) $delay

    set redq [[$ns_ link $node_(r1) $node_(r2)] queue]

## The following controls ECN:
    $redq set setbit_ $ecnmode
    $redq set maxthresh_ 20
        
    # trace only the bottleneck link
    #$self traceQueues $node_(r1) [$self openTrace $stoptime $testName_]
    $ns_ at $stoptime "$self cleanupAll $testName_"
}

TestSuite instproc tcpsetup { tcptype {starttime 0.0} {numpkts 10000} {ssthresh 30} { tcp1fid 0 } { delack 0 }  {src s1} {dst s3} } {
    $self instvar ns_ node_

    if {$tcptype == "Tahoe" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP $node_($src) \
	  TCPSink $node_($dst) $tcp1fid]
    } elseif {$tcptype == "Sack1" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_($src) \
	  TCPSink/Sack1  $node_($dst) $tcp1fid]
    } elseif {$tcptype == "SackRH" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP/SackRH $node_($src) \
	  TCPSink/Sack1  $node_($dst) $tcp1fid]
    } elseif {$tcptype == "SackRHNewReno" && $delack == 0} {
      set tcp1 [$ns_ create-connection TCP/SackRH $node_($src) \
	  TCPSink  $node_($dst) $tcp1fid]
    } elseif {$delack == 0} {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_($src) \
	  TCPSink $node_($dst) $tcp1fid]
    } elseif {$tcptype == "Tahoe" && $delack == 1} {
      set tcp1 [$ns_ create-connection TCP $node_($src) \
	  TCPSink/DelAck $node_($dst) $tcp1fid]
    } elseif {$tcptype == "Sack1" && $delack == 1} {
      set tcp1 [$ns_ create-connection TCP/Sack1 $node_($src) \
	  TCPSink/Sack1/DelAck  $node_($dst) $tcp1fid]
    } elseif {$tcptype == "SackRH" && $delack == 1} {
      set  tcp1 [$ns_ create-connection TCP/SackRH $node_($src) \
	  TCPSink/Sack1/DelAck  $node_($dst) $tcp1fid]
    } else {
      set tcp1 [$ns_ create-connection TCP/$tcptype $node_($src) \
	  TCPSink/DelAck $node_($dst) $tcp1fid]
    } 
    $tcp1 set window_ 100
    $tcp1 set ecn_ 1
    $tcp1 set rtxcur_init_ 3.0
    $ns_ at 0.2 "$tcp1 set ssthresh_ $ssthresh"
    set ftp1 [$tcp1 attach-app FTP]
    $self enable_tracecwnd $ns_ $tcp1
        
#    $self enable_tracequeue $ns_
    $ns_ at $starttime "$ftp1 produce $numpkts"
        
    $self tcpDump $tcp1 5.0

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

TestSuite instproc ecn_pkts pkts {
    $self instvar ns_ errmodel2
    set emod [$self emod2]
    set errmodel2 [new ErrorModel/List]
    $errmodel2 droplist $pkts
    $emod insert $errmodel2
    $emod bind $errmodel2 1
    $errmodel2 set markecn_ true
}

#######################################################################
# All tests
#######################################################################

#  The following set of tests go through a pile of tests for SackRH
#  to make sure that they all work correctly.  


## Single Drop
Class Test/test_sackRH -superclass TestSuite
Test/test_sackRH instproc init {} {
        $self instvar net_ test_ xlimits_ fig_file_
        Queue/RED set setbit_ true
        set net_	net2-lossy
	Agent/TCP set bugFix_ true
        set test_	"SackRH(NewReno)..SackRH..NewReno..Reno"
        set xlimits_     "0,12.0"
        set fig_file_     fig1B.eps
        $self next
}
Test/test_sackRH instproc run {} {
	$self instvar ns_ errmodel1
	Agent/TCP set old_ecn_ 1

	$self netsetup 12.0 true
        $self tcpsetup SackRHNewReno 0.0 150 30 1 0 
        $self tcpsetup SackRH 3.0 150 30 1 0 
        $self tcpsetup Newreno 6.0 150 30 1 0 
        $self tcpsetup Reno 9.0 150 30 1 0 


    puts "Enter loss sequence"
    gets stdin drops
    set offset [expr 150 + [llength $drops]]
        $self drop_pkts [offset_list_3 $drops $offset]

    puts "Enter ecn sequence"
    gets stdin ecns
        $self ecn_pkts [offset_list_3 $ecns $offset]

	$ns_ run
}

proc offset_list {l1 l2} {
    set len1 [llength $l1]
    set len2 [llength $l2]

    for {set i 0} {$i < $len2} {incr i} {
	for {set j 0} {$j < $len1} {incr j} {
	    lappend l1 [expr [lindex $l1 $j] + [lindex $l2 $i]]
	}
    }

    return $l1
}

# This applies the offset 3 times, so we get a total of
# four of the same sequence of packet drops.

proc offset_list_3 {l1 offset} {
    set len1 [llength $l1]

    for {set j 0} {$j < [expr $len1 * 3]} {incr j} {
	lappend l1 [expr [lindex $l1 $j] + $offset]
    }

    return $l1
}


TestSuite runTest
