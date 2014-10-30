#
# Copyright (c) @ Regents of the University of California.
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
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/scuba/noscuba-sat/demo.tcl,v 1.1 1997/06/14 00:16:04 elan Exp $
#

set tcldir ../../../

source $tcldir/rtp/session-scuba.tcl
source $tcldir/rtp/session-rtp.tcl

proc trace_annotate { s } {
	global ns
	set f [$ns set traceAllFile_]
	
	puts $f [format "v %s %s {set sim_annotation {%s}}" [$ns now] eval $s]
}

set ns [new MultiSim]

for { set i 0 } { $i < 8 } { incr i } {
	set node($i) [$ns node]
}

set f [open out.tr w]
$ns trace-all $f

Queue set limit_ 8

proc makelinks { bw delay pairs } {
	global ns node
	foreach p $pairs {
		set src $node([lindex $p 0])
		set dst $node([lindex $p 1])
		$ns duplex-link $src $dst $bw $delay DropTail
	}
}

makelinks 1.5Mb 10ms {
	{ 0 3 }
	{ 1 3 }
	{ 2 3 }
	{ 4 7 }
	{ 5 7 }
	{ 6 7 }
}

makelinks 400kb 50ms {
	{ 3 7 }
}

for { set i 0 } { $i < 8 } { incr i } {
	set dm($i) [new DM $ns $node($i)]
}

$ns at 0.0 "$ns run-mcast"

set sessbw 400kb/s

foreach n { 0 1 2 4 5 6 } {
	set sess($n) [new Session/RTP]
	$sess($n) session_bw $sessbw
	$sess($n) attach-node $node($n)
}

$ns at 1.0 {
	global sess
	foreach n { 0 1 2 4 5 6 } {
		$sess($n) join-group 0x8000
	}
}

foreach n { 0 1 2 } {
	set d [$sess($n) set dchan_]
	$d set class_ [expr $n]
}

$ns at 1.0 "trace_annotate {Starting receivers...}"
# start receivers
$ns at 1.0 {
	global sess
	$sess(4) start
	$sess(5) start
	$sess(6) start
}

# start senders
$ns at 1.0 "trace_annotate {Starting sender 1 at 200kb/s...}"
$ns at 1.0 "$sess(0) start"
$ns at 1.0 "$sess(0) transmit 200kb/s"

$ns at 2.0 "trace_annotate {Starting sender 2 at 200kb/s...}"
$ns at 2.0 "$sess(1) start"
$ns at 2.0 "$sess(1) transmit 200kb/s
"
$ns at 3.0 "trace_annotate {Starting sender 3 at 200kb/s...}"
$ns at 3.0 "$sess(2) start"
$ns at 3.0 "$sess(2) transmit 200kb/s"

$ns at 5.0 "finish"

proc finish {} {
	global tcldir
	puts "converting output to nam format..."
        global ns
        $ns flush-trace
	exec awk -f $tcldir/nam-demo/nstonam.awk out.tr > demo-nam.tr
	exec rm -f out
        #XXX
	puts "running nam..."
	exec /usr/local/src/nam/nam demo-nam &
        exit 0
}

$ns run

