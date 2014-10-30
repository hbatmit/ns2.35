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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/test/misc.tcl,v 1.20 2000/05/24 00:22:27 heideman Exp $
#

#source plotting.tcl

if [file exists redefines.tcl] {
	puts "sourcing redefines.tcl in [pwd]"
	source redefines.tcl
}

Object instproc exit args {
      set ns [Simulator instance]
      catch "$ns clearTimers"
      eval exit $args
}

Class TestSuite

TestSuite instproc init { {dotrace 1} } {
	global quiet
	$self instvar ns_ net_ defNet_ test_ topo_ node_ testName_ 
	$self instvar allchan_ namchan_
	if [catch "$self get-simulator" ns_] {
	    set ns_ [new Simulator]
	}
	if { $dotrace } {
                set allchan_ [open all.tr w]
                $ns_ trace-all $allchan_
		set namchan_ [open all.nam w]
		if {$quiet == "false"} {
                	$ns_ namtrace-all $namchan_
		}
	}
	if {$net_ == ""} {
		set net_ $defNet_
	}
	if ![Topology/$defNet_ info subclass Topology/$net_] {
		global argv0
		puts "$argv0: cannot run test $test_ over topology $net_"
		exit 1
	}

	set topo_ [new Topology/$net_ $ns_]
	foreach i [$topo_ array names node_] {
		# This would be cool, but lets try to be compatible
		# with test-suite.tcl as far as possible.
		#
		# $self instvar $i
		# set $i [$topo_ node? $i]
		#
		set node_($i) [$topo_ node? $i]
	}

	if {$net_ == $defNet_} {
		set testName_ "$test_"
	} else {
		set testName_ "$test_:$net_"
	}

	if { $dotrace } {
		# XXX
		if [info exists node_(k1)] {
			set blink [$ns_ link $node_(r1) $node_(k1)]
		} else {
			set blink [$ns_ link $node_(r1) $node_(r2)] 
		}
		$blink trace-dynamics $ns_ stdout 
	}
}

TestSuite instproc finish file {
#	global env
#
#	THIS CODE IS NOW SUPERSEDED BY THE NEWER EXTERNAL DRIVERS,
#	raw2xg, and raw2gp, in ~ns/bin.  raw2xg generates output suitable
#	for xgraph, and raw2gp, that suitable for gnuplot.
#
#       To reproduce old functionality:
#	global PERL
#	exec $PERL ../../bin/getrc -s 2 -d 3 all.tr | \
#	  $PERL ../../bin/raw2xg -s 0.01 -m 90 |  \
#	  xgraph -bb -tk -nl -m -x time -y packets"
#	
#       catch "$self exit 0"
	exit 0
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

TestSuite instproc tcpDumpAll { tcpSrc interval label } {
	$self instvar dump_inst_ ns_
	if ![info exists dump_inst_($tcpSrc)] {
		set dump_inst_($tcpSrc) 1
		puts $label/window=[$tcpSrc set window_]/packetSize=[$tcpSrc set packetSize_]/bugFix=[$tcpSrc set bugFix_]	
		$ns_ at 0.0 "$self tcpDumpAll $tcpSrc $interval $label"
		return
	}
	$ns_ at [expr [$ns_ now] + $interval] "$self tcpDumpAll $tcpSrc $interval $label"
	puts $label/time=[$ns_ now]/cwnd=[format "%.4f" [$tcpSrc set cwnd_]]/ssthresh=[$tcpSrc set ssthresh_]/ack=[$tcpSrc set ack_]/rtt=[$tcpSrc set rtt_]	
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

TestSuite instproc namtraceQueues { node traceFile } {
	$self instvar ns_
	foreach nbr [$node neighbors] {
		$ns_ namtrace-queue $node $nbr $traceFile
		[$ns_ link $node $nbr] trace-dynamics $ns_ $traceFile "nam"
	}
}

proc usage {} {
	global argv0
	puts stderr "usage: ns $argv0 <tests> \[<topologies>\]"
	puts stderr "Valid tests are:\t[get-subclasses TestSuite Test/]"
	puts stderr "Valid Topologies are:\t[get-subclasses SkelTopology Topology/]"
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

                        set topo ""
                }
                2 {
                        set test [lindex $argv 0]
                        isProc? Test $test

                        set topo [lindex $argv 1]
                        if {$topo == "QUIET"} {
                                set quiet true 
                                set topo ""
                        } else {
                                isProc? Topology $topo
                        }
                }
                3 {
                        set test [lindex $argv 0]
                        isProc? Test $test

                        set topo [lindex $argv 1]
                        isProc? Topology $topo

                        set extra [lindex $argv 2]
                        if {$extra == "QUIET"} {
                                set quiet true
                        }
                }
                default {
                        usage
                }
        }
        set t [new Test/$test $topo]
        $t run
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 8
### tcl-default-application: ns
### End:
