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

# Test with 1000 nodes in the detailed network
# Create simulation topology
#     DN==============AN
#     1
#      \
#        -- 0 ======= 1001
#      /
#     1000
proc create-topo { num_node } {
    global ns n an_id verbose
	
    for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
    }

    # setup links within detailed networks, use default routing
    for {set i 1} {$i < [expr $num_node - 1]} {incr i} {
	$ns duplex-link $n(0) $n($i) 25Mb 10ms DropTail
	# default routing
	[$n($i) entry] defaulttarget [[$ns link $n($i) $n(0)] head]

	if {$verbose} { 
	    puts "(0 $i) 25Mb 10ms DropTail"
	}
    }

    # setup link between DN and AN
    $ns duplex-link $n(0) $n($an_id) 100Mb 20ms DropTail
    [$n(0) entry] defaulttarget [[$ns link $n(0) $n($an_id)] head]
    if {$verbose} { 
	puts "(0 $an_id) 100Mb 20ms DropTail"
    }
}

set ON 1
set OFF 0

# enable debugging by default
set verbose ON

# Initialize simulator
set ns [new Simulator]

set num_node 502
set an_id [expr $num_node - 1]
create-topo $num_node

# set up simulation scenario
set total_host 200000

set probing_port [Application/Worm set ScanPort]
# probing packet size
# slammer worm UDP packets of 404 Bytes
set p_size 404
Application/Worm set ScanPacketSize $p_size
Agent/MessagePassing set packetSize_ $p_size

#	Application/Worm set ScanRate 1

# 1.config the detailed network
for {set i 0} {$i < $num_node - 1} {incr i} {
    set a($i) [new Agent/MessagePassing]
    $n($i) attach $a($i) $probing_port
    set w($i) [new Application/Worm/Dnh]
    $w($i) attach-agent $a($i)
    $w($i) addr-range 0 [expr $num_node - 2]
}

# 2. set up abstract network
set a($an_id) [new Agent/MessagePassing]
$n($an_id) attach $a($an_id) $probing_port
set w($an_id) [new Application/Worm/An]
$w($an_id) attach-agent $a($an_id)
$w($an_id) addr-range $an_id $total_host
$w($an_id) dn-range 0 [expr $num_node - 2]
$w($an_id) v_percent 0.8

# recv all packets coming in
set p [$n($an_id) set dmux_]
$p defaulttarget $a($an_id)
[$n($an_id) entry] defaulttarget $p

# now starts infection
$ns at 1.0 "$w($an_id) start"

proc finish {} {
    exit 0
}

$ns at 65.0 "finish"

$ns run


