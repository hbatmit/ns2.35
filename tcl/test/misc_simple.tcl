
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/misc_simple.tcl,v 1.13 2003/01/19 03:51:18 sfloyd Exp $
#

Object instproc exit args {
      set ns [Simulator instance]
      catch "$ns clearTimers"
      eval exit $args
}

Class TestSuite

# Use "$self next 0" or "$self next noTraceFiles" to avoid creating 
#   all.tr and all.nam.
# Use "$self next 2" or "$self next pktTraceFile" to create only all.tr, 
#   but not all.nam.
TestSuite instproc init { {dotrace traceFiles} } {
	global quiet argv0
	$self instvar ns_ test_ node_ testName_ 
	$self instvar allchan_ namchan_
	if [catch "$self get-simulator" ns_] {
	    set ns_ [new Simulator]
	}
	if { $dotrace == 1 || $dotrace == 2 || $dotrace == "traceFiles" || 
		   $dotrace == "pktTraceFile" } {
                set allchan_ [open all.tr w]
                $ns_ trace-all $allchan_
        } 
        if { $dotrace == "traceFiles" || $dotrace == 1 } {
		set namchan_ [open all.nam w]
		if {$quiet == "false"} {
                	$ns_ namtrace-all $namchan_
		}
		if {[regexp {testReno} $argv0]} {
			$ns_ eventtrace-all
		}
	}
	set testName_ "$test_"
}

#
# Arrange for tcp source stats to be dumped for $tcpSrc every
# $interval seconds of simulation time
#
TestSuite instproc tcpDump { tcpSrc interval } {
	global quiet
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		$ns_ at 0.0 "$self tcpDump $tcpSrc $interval"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self tcpDump $tcpSrc $interval"
	set report [$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]
        if {$quiet == "false"} {
                puts $report
        }
}

#
# Arrange for time to be printed every
# $interval seconds of simulation time
#
TestSuite instproc timeDump { interval } {
	global quiet
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_] {
		set dump_inst_ 1
		$ns_ at 0.0 "$self timeDump $interval"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self timeDump $interval"
	set report [$ns_ now]
        if {$quiet == "false"} {
                puts $report
        }
}

#
# Trace the TCP congestion window cwnd_.
#
TestSuite instproc enable_tracecwnd { ns tcp {filename all.cwnd} } { 
        $self instvar cwnd_chan_
        set cwnd_chan_ [open $filename w]
        $tcp trace cwnd_
        $tcp attach $cwnd_chan_ 
}       
        
#
# Plot the TCP congestion window cwnd_.
#
TestSuite instproc plot_cwnd { {terse 0} {title cwnd} {newfiles 0} } {
        global quiet
        $self instvar cwnd_chan_
        set awkCode {
              {
              if ($6 == "cwnd_") {
                print $1, $7 >> "temp.cwnd";
              } }
        }
        set awkCodeTerse {
	      BEGIN { oldcwnd = -2; print "  " >> "temp.cwnd";}
              {
              if ($6 == "cwnd_") {
		 newcwnd = $7;
		 newtime = $1;
		 if (newtime < oldtime) {
                    print "  " >> "temp.cwnd";
		    oldcwnd = -1;
		 }
                 if ((newcwnd >= oldcwnd + 1) || (newcwnd <= oldcwnd - 1)){
                    print newtime, newcwnd >> "temp.cwnd";
		    oldcwnd = $7;
		 }
		 oldtime = $1;
              } }
        }
        set f [open cwnd.xgr w]
        puts $f "TitleText: $title"
        puts $f "Device: Postscript"
 
        if { [info exists cwnd_chan_] } {
                close $cwnd_chan_  
        }
        exec rm -f temp.cwnd
        exec touch temp.cwnd
        
	if {$terse == 1} {
		exec awk $awkCodeTerse all.cwnd
		if {$newfiles != 0} {
 		        exec awk $awkCodeTerse all.cwnd1
		}
	} else {
       		exec awk $awkCode all.cwnd
	}
        
        puts $f \"cwnd
        exec cat temp.cwnd >@ $f
        close $f                
        if {$quiet == "false"} {
                exec xgraph -bb -tk -x time -y cwnd cwnd.xgr &
        }               
}                               
                
TestSuite instproc cleanup { tfile testname } {
	$self instvar ns_ allchan_ namchan_
	$ns_ halt
	close $tfile
	if { [info exists allchan_] } {
		close $allchan_
	}       
	if { [info exists namchan_] } {
		close $namchan_
	}       
	$self finish $testname; # calls finish procedure in test suite file
}

TestSuite instproc cleanupAll { testname {stoptime 0}} {
	$self instvar ns_ allchan_ namchan_
	$ns_ halt
	if { [info exists allchan_] } {
		close $allchan_
	}       
	if { [info exists namchan_] } {
		close $namchan_
	}       
	if { $stoptime > 0 } {
		$self finish $testname $stoptime; 
	} else {
		$self finish $testname;
	}
	# calls finish procedure in test suite file
}


TestSuite instproc openTrace { stopTime testName } {
	$self instvar ns_ allchan_ namchan_
	exec rm -f out.tr temp.rands
	set traceFile [open out.tr w]
	puts $traceFile "v testName $testName"
	$ns_ at $stopTime "$self cleanup $traceFile $testName"
	return $traceFile
}

TestSuite instproc traceQueues { node traceFile } {
        $self instvar ns_
        foreach nbr [$node neighbors] { 
                $ns_ trace-queue $node $nbr $traceFile
                [$ns_ link $node $nbr] trace-dynamics $ns_ $traceFile
        }
}
 
proc usage {} {
	global argv0
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	exit 1
}

proc isProc? {cls prc} {
	if [catch "Object info subclass $cls/$prc" r] {
		global argv0
		puts stderr "$argv0: no such $cls: $prc"
		usage
	}
}

proc get-subclasses {cls pfx} {
	set ret ""
	set l [string length $pfx]

	set c $cls
	while {[llength $c] > 0} {
		set t [lindex $c 0]
		set c [lrange $c 1 end]
		if [string match ${pfx}* $t] {
			lappend ret [string range $t $l end]
		}
		eval lappend c [$t info subclass]
	}
	set ret
}

TestSuite proc runTest {} {
        global argc argv quiet

        set quiet false
        switch $argc {
                1 {
                        set test $argv
                        isProc? Test $test
                }
                2 {
                        set test [lindex $argv 0]
                        isProc? Test $test

                        set param [lindex $argv 1]
                        if {$param == "QUIET"} {
                                set quiet true 
                        } 
                }
                default {
                        usage
                }
        }
        set t [new Test/$test]
        $t run
}

TestSuite instproc setTopo {} {
    $self instvar node_ net_ ns_ topo_

    set topo_ [new Topology/$net_ $ns_]
    foreach i [$topo_ array names node_] {
        set node_($i) [$topo_ node? $i]
    }
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
