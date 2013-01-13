#Copyright (c) 1997 Regents of the University of California.
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
#      This product includes software developed by the Computer Systems
#      Engineering Group at Lawrence Berkeley Laboratory.
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
# Each agent keeps track of what messages it has seen
# and only forwards those which it hasn't seen before.

# Each message is of the form "ID:DATA" where ID is some arbitrary
# message identifier and DATA is the payload.  In order to reduce
# memory usage, the agent stores only the message ID.

# Note that I have not put in any mechanism to expire old message IDs
# from the list of seen messages.  There also isn't any mechanism
# for assigning message IDs.


Mac/Simple set bandwidth_ 1Mb

set MESSAGE_PORT 42
set BROADCAST_ADDR -1


# variables which control the number of nodes and how they're grouped
# (see topology creation code below)
set group_size 4
set num_groups 6
set num_nodes [expr $group_size * $num_groups]


set val(chan)           Channel/WirelessChannel    ;#Channel Type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)          Phy/WirelessPhy            ;# network interface type



#set val(mac)            Mac/802_11                 ;# MAC type
#set val(mac)            Mac                 ;# MAC type
set val(mac)		Mac/Simple


set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq


# DumbAgent, AODV, and DSDV work.  DSR is broken
set val(rp) DumbAgent
#set val(rp)             DSDV
#set val(rp)             DSR
#set val(rp)		 AODV


# size of the topography
set val(x)              [expr 120*$group_size + 500]
set val(y)              [expr 240*$num_groups + 200]



set ns [new Simulator]

set f [open wireless-flooding-$val(rp).tr w]
$ns trace-all $f
set nf [open wireless-flooding-$val(rp).nam w]
$ns namtrace-all-wireless $nf $val(x) $val(y)

$ns use-newtrace

# set up topography object
set topo       [new Topography]

$topo load_flatgrid $val(x) $val(y)

#
# Create God
#
create-god $num_nodes



set chan_1_ [new $val(chan)]

$ns node-config -adhocRouting $val(rp) \
                -llType $val(ll) \
                -macType $val(mac) \
                -ifqType $val(ifq) \
                -ifqLen $val(ifqlen) \
                -antType $val(ant) \
                -propType $val(prop) \
                -phyType $val(netif) \
                -topoInstance $topo \
                -agentTrace ON \
                -routerTrace OFF \
                -macTrace ON \
                -movementTrace OFF \
                -channel $chan_1_ 


# subclass Agent/MessagePassing to make it do flooding

Class Agent/MessagePassing/Flooding -superclass Agent/MessagePassing

Agent/MessagePassing/Flooding instproc recv {source sport size data} {
    $self instvar messages_seen node_
    global ns BROADCAST_ADDR 

    # extract message ID from message
    set message_id [lindex [split $data ":"] 0]
    puts "\nNode [$node_ node-addr] got message $message_id\n"

    if {[lsearch $messages_seen $message_id] == -1} {
	lappend messages_seen $message_id
        $ns trace-annotate "[$node_ node-addr] received {$data} from $source"
        $ns trace-annotate "[$node_ node-addr] sending message $message_id"
	$self sendto $size $data $BROADCAST_ADDR $sport
    } else {
        $ns trace-annotate "[$node_ node-addr] received redundant message $message_id from $source"
    }
}

Agent/MessagePassing/Flooding instproc send_message {size message_id data port} {
    $self instvar messages_seen node_
    global ns MESSAGE_PORT BROADCAST_ADDR

    lappend messages_seen $message_id
    $ns trace-annotate "[$node_ node-addr] sending message $message_id"
    $self sendto $size "$message_id:$data" $BROADCAST_ADDR $port
}



# create a bunch of nodes
for {set i 0} {$i < $num_nodes} {incr i} {
    set n($i) [$ns node]
    $n($i) set Y_ [expr 230*floor($i/$group_size) + 160*(($i%$group_size)>=($group_size/2))]
    $n($i) set X_ [expr (90*$group_size)*($i/$group_size%2) + 200*($i%($group_size/2))]
    $n($i) set Z_ 0.0
    $ns initial_node_pos $n($i) 20
}



# attach a new Agent/MessagePassing/Flooding to each node on port $MESSAGE_PORT
for {set i 0} {$i < $num_nodes} {incr i} {
    set a($i) [new Agent/MessagePassing/Flooding]
    $n($i) attach  $a($i) $MESSAGE_PORT
    $a($i) set messages_seen {}
}


# now set up some events
$ns at 0.2 "$a(1) send_message 200 1 {first message}  $MESSAGE_PORT"
$ns at 0.4 "$a([expr $num_nodes/2]) send_message 600 2 {some big message} $MESSAGE_PORT"
$ns at 0.7 "$a([expr $num_nodes-2]) send_message 200 3 {another one} $MESSAGE_PORT"

$ns at 1.0 "finish"

proc finish {} {
        global ns f nf val
        $ns flush-trace
        close $f
        close $nf

#        puts "running nam..."
        exec nam wireless-flooding-$val(rp).nam &
        exit 0
}

$ns run

