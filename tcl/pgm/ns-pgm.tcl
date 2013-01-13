#
# Copyright (c) 2001 University of Southern California.
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
##
# Pragmatic General Multicast (PGM), Reliable Multicast
#
# ns-pgm.tcl
#
# Auxillary OTcl procedures required by the PGM implementation, these are
# used to set up Agent/PGM on new nodes when PGM is activated.
#
# Code adapted from Christos Papadopoulos.
#
# Ryan S. Barnett, 2001
#

# These default values should stay here since the pgm module might be disabled when option --disable-stl is used during configuration.
################################################################
# PGM
################################################################

RtModule/PGM set node_  ""

PGMErrorModel set rate_         0.0     ;# just to eliminate warnings
PGMErrorModel set errPkt_       0
PGMErrorModel set errByte_      0
PGMErrorModel set errTime_      0.0
PGMErrorModel set onlink_       0
PGMErrorModel set delay_        0
PGMErrorModel set delay_pkt_    0
PGMErrorModel set enable_       0
PGMErrorModel set ndrops_       0
PGMErrorModel set bandwidth_    2Mb
PGMErrorModel set markecn_      false
PGMErrorModel set debug_        false

# *** PGM AGENT ***
Agent/PGM set pgm_enabled_ 1

# Number of seconds to wait between retransmitting a NAK that is waiting
# for a NCF packet.
Agent/PGM set nak_retrans_ival_ 50ms

# The length of time for which a network element will continue to repeat
# NAKs while waiting for a corresponding NCF.  Once this time expires and
# no NCF is received, then we remove the entire repair state.
Agent/PGM set nak_rpt_ival_ 1000ms

# The length of time for which a network element will wait for the
# corresponding RDATA before removing the entire repair state.
Agent/PGM set nak_rdata_ival_ 10000ms

# Once a NAK has been confirmed, network elements must discard all
# further NAKs for up to this length of time.  Should be a fraction
# of nak_rdata_ival_.
Agent/PGM set nak_elim_ival_ 5000ms

Agent/PGM instproc done {} { }

# *** PGM SENDER ***

# The length of time between sending SPM packets.
Agent/PGM/Sender set spm_interval_ 500ms

# Time to delay sending out an RDATA in response to a NAK packet, this
# is to allow slow NAKs to get processed at one time, so we don't send
# out duplicate RDATA.
Agent/PGM/Sender set rdata_delay_ 70ms

Agent/PGM/Sender instproc done {} { }

# *** PGM RECEIVER ***

# Maximum number of times we can send out a NAK and time-out waiting for
# an NCF reply. Once we hit this many times, we discard the NAK state
# entirely and loose data.
Agent/PGM/Receiver set max_nak_ncf_retries_ 5

# Maximum number of times we can time-out waiting for RDATA after an
# NCF confirmation for a NAK request.  Once we hit this many times, we
# discard the NAK state entirely and loose data.
Agent/PGM/Receiver set max_nak_data_retries_ 5

# A random amount of this time period (range) that will be selected to wait
# for an NCF after detecting a gap in the data stream, before sending out
# a NAK.
Agent/PGM/Receiver set nak_bo_ivl_ 30ms

# The amount of time to wait for a NCF packet after sending out a NAK
# packet to the upstream node. If no NCF is received, another random
# backoff time is observed, and then the NAK is retransmitted.
Agent/PGM/Receiver set nak_rpt_ivl_ 50ms

# The amount of time to wait for RDATA after receiving an NCF confirmation
# for a given NAK. Once this timer expires, another random backoff time
# is observed, and then the NAK is retransmitted.
Agent/PGM/Receiver set nak_rdata_ivl_ 1000ms

Agent/PGM/Receiver instproc done {} { }


#
# Register this PGM agent with a node.
# Create and attach an PGM classifier to node.
# Redirect node input to PGM classifier and
# classifier output to either PGM agent or node
#

#Class RtModule/PGM -superclass RtModule

RtModule/PGM instproc register { node } {
	$self instvar node_ pgm_classifier_

	set node_ $node
	set pgm_classifier_ [new Classifier/Pgm]
	set pgm_agent [new Agent/PGM]
	$node attach $pgm_agent
        $node set-pgm $pgm_agent
	$node insert-entry $self $pgm_classifier_ 0
	$pgm_classifier_ install 1 $pgm_agent
}

RtModule/PGM instproc get-outlink { iface } {
	$self instvar node_

	set oif [$node_ iif2oif $iface]
	#set outlink [$node_ oif2link $oif]
	return $oif    
}

Node instproc ifaceGetOutLink { iface } {
	$self instvar ns_
	set link [$self iif2link $iface]
	set outlink [$ns_ link $self [$link src]]
	set head [$outlink set head_]
	return $head
}

#Node instproc ifaceGetOutLink { iface } {
#        $self instvar ns_ id_ neighbor_
#        foreach node $neighbor_ {
#                set link [$ns_ set link_([$node id]:$id_)]
#            if {[[$link set ifaceout_] id] == $iface} {
#                set olink [$ns_ set link_($id_:[$node id])]
#                set head [$olink set head_]
#                return $head
#            }
#        }
#        return -1
#}

Node instproc set-switch agent {
	$self instvar switch_
	set switch_ $agent
}

Node instproc agent port {
        $self instvar agents_
        foreach a $agents_ {
		#puts "the agent at node [$self id] is $a"
                if { [$a set agent_port_] == $port } {
			#puts "node: [$self id], port:$port, agent:$a"
                        return $a
                }
        }
        return ""
}

# Set the Agent/PGM for a particular node.
Node instproc set-pgm agent {
    $self instvar pgm_agent_
    set pgm_agent_ $agent
}

# Retrieve the Agent/PGM from a particular node.
Node instproc get-pgm {} {
    $self instvar pgm_agent_
    return $pgm_agent_
}

# Agent/LMS/Receiver instproc log-loss {} {
# }

#
# detach a lossobj from link(from:to)
#
Simulator instproc detach-lossmodel {lossobj from to} {
	set link [$self link $from $to]
	set head [$link head]
	$head target [$lossobj target]
}

Agent/PGM/Sender instproc init {} {
    eval $self next
    set ns [Simulator instance]
    $ns create-eventtrace Event $self
}

Agent/PGM/Receiver instproc init {} {
    eval $self next
    set ns [Simulator instance]
    $ns create-eventtrace Event $self
}

Agent/PGM instproc init {} {
    eval $self next
    set ns [Simulator instance]
    $ns create-eventtrace Event $self
}

 
