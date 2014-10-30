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
# Light-Weight Multicast Services (LMS), Reliable Multicast
#
# ns-lms.tcl
#
# Auxillary OTcl procedures required by the LMS implementation, these are
# used to set up Agent/LMS on new nodes when LMS is activated.
#
# Christos Papadopoulos.
# christos@isi.edu
#

################################################################
# LMS
################################################################

#
# Register this Lms agent with a node.
# Create and attach an lms classifier to node.
# Redirect node input to lms classifier and
# classifier output to either lms agent or node
#
RtModule/LMS instproc register { node } {
	$self instvar node_ lms_classifier_

#puts "****In Lms register for node [$node id]"
	set node_ $node
	set lms_classifier_ [new Classifier/Lms]
	set lms_agent [new Agent/LMS]
	$node attach $lms_agent
	$node insert-entry $self $lms_classifier_ 0
	$lms_classifier_ install 1 $lms_agent
}

RtModule/LMS instproc get-outlink { iface } {
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

Agent/LMS/Receiver instproc log-loss {} {
}

#
# detach a lossobj from link(from:to)
#
Simulator instproc detach-lossmodel {lossobj from to} {
	set link [$self link $from $to]
	set head [$link head]
	$head target [$lossobj target]
} 
