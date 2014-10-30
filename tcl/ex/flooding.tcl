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
# from the list of seen messages.  There also isn't any standard mechanism
# for assigning message IDs.  An actual assignment may wish to have the
# students come up with solutions for these problems.


set MESSAGE_PORT 42


# parameters for topology generator
set group_size 7
set num_groups 5
set num_nodes [expr $group_size * $num_groups]


set ns [new Simulator]

set f [open flooding.tr w]
$ns trace-all $f
set nf [open flooding.nam w]
$ns namtrace-all $nf



# subclass Agent/MessagePassing to make it do flooding

Class Agent/MessagePassing/Flooding -superclass Agent/MessagePassing

Agent/MessagePassing/Flooding instproc send_message {size msgid msg} {
    $self instvar messages_seen node_
    global ns MESSAGE_PORT

    $ns trace-annotate "Node [$node_ node-addr] is sending {$msgid:$msg}"

    lappend messages_seen $msgid
    $self send_to_neighbors -1 $MESSAGE_PORT $size "$msgid:$msg"
}

Agent/MessagePassing/Flooding instproc send_to_neighbors {skip port size data} {
    $self instvar node_

	foreach x [$node_ neighbors] {
	    set addr [$x set address_]
	    if {$addr != $skip} {
		$self sendto $size $data $addr $port
	    }
	}
}

Agent/MessagePassing/Flooding instproc recv {source sport size data} {
    $self instvar messages_seen node_
    global ns

    # extract message ID from message
    set message_id [lindex [split $data ":"] 0]

    if {[lsearch $messages_seen $message_id] == -1} {
	lappend messages_seen $message_id
        $ns trace-annotate "Node [$node_ node-addr] received {$data}"
	$self send_to_neighbors $source $sport $size $data
    } else {
	$ns trace-annotate "Node [$node_ node-addr] received redundant copy of message #$message_id"
    }
}


## Topology Generator

# create a bunch of nodes
for {set i 0} {$i < $num_nodes} {incr i} {
    set n($i) [$ns node]
}

# create links between the nodes
for {set g 0} {$g < $num_groups} {incr g} {
    for {set i 0} {$i < $group_size} {incr i} {
        $ns duplex-link $n([expr $g*$group_size+$i]) $n([expr $g*$group_size+($i+1)%$group_size]) 2Mb 15ms DropTail
    }
    $ns duplex-link $n([expr $g*$group_size]) $n([expr (($g+1)%$num_groups)*$group_size+2]) 2Mb 15ms DropTail
    if {$g%2} {
        $ns duplex-link $n([expr $g*$group_size+3]) $n([expr (($g+3)%$num_groups)*$group_size+1]) 2Mb 15ms DropTail
    }
}


# attach a new Agent/MessagePassing/Flooding to each node on port $MESSAGE_PORT
for {set i 0} {$i < $num_nodes} {incr i} {
    set a($i) [new Agent/MessagePassing/Flooding]
    $n($i) attach  $a($i) $MESSAGE_PORT
    $a($i) set messages_seen {}
}


# now set up some events
$ns at 0.2 "$a(5) send_message 900 1 {first message}"
$ns at 0.5 "$a(17) send_message 700 2 {another one}"
$ns at 1.0 "$a(24) send_message 500 abc {yet another one}"

$ns at 2.0 "finish"

proc finish {} {
        global ns f nf
        $ns flush-trace
        close $f
        close $nf

        puts "running nam..."
        exec nam flooding.nam &
        exit 0
}

$ns run

