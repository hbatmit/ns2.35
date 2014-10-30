#
# Copyright (c) 1997, 1998 Regents of the University of California.
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
# 	This product includes software developed by the Daedalus Research
#       Group at the University of California, Berkeley.
# 4. Neither the name of the University nor of the research group
#    may be used to endorse or promote products derived from this software 
#    without specific prior written permission.
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
# Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
#
# Defaults for link-layer
LL set bandwidth_ 0      ;# not used
LL set delay_ 1ms
LL set macDA_ 0

LL/Sat/HDLC set window_size_ 8
LL/Sat/HDLC set queue_size_ 1000
LL/Sat/HDLC set timeout_ 0.26
LL/Sat/HDLC set max_timeouts_ 2 ;# other values 3, 5, 50, 200 maybe set from user tcl script

LL/Sat/HDLC set delAck_   false  ;# ack maybe delayed to allow piggybacking
LL/Sat/HDLC set delAckVal_  0.1  ;# value chosen based on a link delay of 100ms
LL/Sat/HDLC set selRepeat_ false ;# set to GoBackN by default

if [TclObject is-class LL/Arq] {
LL/Arq set mode_ 2
LL/Arq set hlen_ 16
LL/Arq set slen_ 1400
LL/Arq set limit_ 8
LL/Arq set timeout_ 100ms

Class LL/Rlp -superclass LL/Arq
LL/Rlp set mode_ 1
LL/Rlp set hlen_ 6
LL/Rlp set slen_ 30
LL/Rlp set limit_ 63
LL/Rlp set timeout_ 500ms
LL/Rlp set delay_ 70ms
}


# Snoop variables
if [TclObject is-class Snoop] {
	Snoop set snoopTick_ 0.1
	Snoop set snoopDisable_ 0
	Snoop set srtt_ 0.1
	Snoop set rttvar_ 0.25
	Snoop set g_ 0.125
	Snoop set tailTime_ 0
	Snoop set rxmitStatus_ 0
	Snoop set lru_ 0
	Snoop set maxbufs_ 0
}

if [TclObject is-class LL/LLSnoop] {
	LL/LLSnoop set integrate_ 0
	LL/LLSnoop set delay_ 0ms
	Snoop set srtt_ 0.1
	Snoop set rttvar_ 0.25
	Snoop set g_ 0.125
	LL/LLSnoop set snoopTick_ 0.1
}

# Get snoop agent (or else create one automatically
LL/LLSnoop instproc get-snoop { src dst } {
	$self instvar snoops_ off_ll_ delay_
	
	if { ![info exists snoops_($src:$dst)] } {
		# make a new snoop agent if none exists
#		puts "making new snoop $src $dst"
		set snoops_($src:$dst) [new Snoop]
	}
	# make snoop's llsnoop_ ourself, and make it's recvtarget_ same as ours
	$snoops_($src:$dst) llsnoop $self
	$snoops_($src:$dst) set delay_ $delay_
	return $snoops_($src:$dst)
}

# Integrate snoop processing across concurrent active connections
LL/LLSnoop instproc integrate { src dst } {
	$self instvar snoops_

	set conn $src:$dst
	if {![info exists snoops_($conn)]} {
		return
	}

	set snoop $snoops_($conn)
	set threshtime [$snoop set tailTime_]

	foreach a [array names snoops_] {
		# go through other conns checking for rxmit possibilities
		if { $a != $conn } {
			$snoops_($a) check-rxmit $threshtime
			if { [$snoops_($a) set rxmitStatus_] == 2 } {
				# We just did a retransmission, don't be
				# too aggressive on rxmissions.  This follows
				# standard conservation of packets.
				break;
			}
		}
	}
}









