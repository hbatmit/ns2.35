#
# Copyright (c) 1995 Regents of the University of California.
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
#	This product includes software developed by the Network Research
#	Group at Lawrence Berkeley Laboratory.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/example.tcl,v 1.5 2001/07/13 00:11:55 mehringe Exp $ (LBL)
#

#
# Create two nodes and connect them with a 1.5Mb link with a
# transmission delay of 10ms using FIFO drop-tail queueing
#
set ns [new Simulator]
set n0 [$ns node]
set n1 [$ns node]
$ns duplex-link $n0 $n1 1.5Mb 10ms DropTail

#
# Set up BSD Tahoe TCP connection in opposite directions.
#
set src1 [$ns create-connection TCP $n0 TCPSink $n1 1]
set src2 [$ns create-connection TCP $n1 TCPSink $n0 2]

#
# Create ftp sources at the each node
#
set ftp1 [$src1 attach-app FTP]
set ftp2 [$src2 attach-app FTP]

#
# Start up the first ftp at the time 0 and
# the second ftp staggered 1 second later
#
$ns at 0.0 "$ftp1 start"
$ns at 1.0 "$ftp2 start"

#
# Create a trace and arrange for all link
# events to be dumped to "out.tr"
#
set tf [open out.tr w]
$ns trace-queue $n0 $n1 $tf
set qmon [$ns monitor-queue $n0 $n1 ""]
set integ [$qmon get-bytes-integrator]

#
# Dump the queueing delay on the n0->n1 link
# to stdout every second of simulation time.
#
proc dump { link interval } {
	global ns integ
	$ns at [expr [$ns now] + $interval] "dump $link $interval"
	set delay [expr 8 * [$integ set sum_] / [[$link link] set bandwidth_]]
	puts "[$ns now] delay=$delay"
}
$ns at 0.0 "dump [$ns link $n0 $n1] 1"

#
# run the simulation for 20 simulated seconds
#
$ns at 20.0 "$ns halt; exit 0"
$ns run

#
# For more examples, see the examples and test suite subdirectories
# (tcl/ex and tcl/test)
# and the on-line documentation (at <http://www.isi.edu/nsnam/ns/>).
#
