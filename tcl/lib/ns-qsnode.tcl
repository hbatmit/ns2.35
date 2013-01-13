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
# Quick Start for TCP and IP.
# A scheme for transport protocols to dynamically determine initial 
# congestion window size.
#
# http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
#
# ns-qsnode.tcl
#
# OTcl support procedures required for Quick Start agent
#
# Srikanth Sundarrajan, 2002
# sundarra@usc.edu
#

RtModule/QS instproc register { node } {
	$self next $node

	$self instvar classifier_ 
	# Copy the old classifier and save it.  
	$self set classifier_ [$node entry]

	# Create a new classifer for qs processing and attach qs agent
	$node set qs_classifier_ [new Classifier/QS]
	$node set qs_agent_ [new Agent/QSAgent]
	$node set switch_ [$node set qs_classifier_]

	# Install existing classifier at slot 1, new classifier at slot 0
	$node insert-entry $self [$node set switch_] 1

	[$node set switch_] install 0 [$node set qs_agent_]
	$node attach [$node set qs_agent_]
	[$node set qs_agent_] set old_classifier_ $classifier_

}

Simulator instproc QS { val } {
	if { $val == "ON" } {
		add-packet-header "TCP_QS"
		Node enable-module "QS"
	} else {
		Node disable-module "QS"
		remove-packet-header "TCP_QS"
	}
}

Node instproc qs-agent {} {
	$self instvar qs_agent_
	return $qs_agent_
}

Simulator instproc get-queue { addr daddr } {
	$self instvar routingTable_
    
	set srcid [$self get-node-id-by-addr $addr]
	set dstid [$self get-node-id-by-addr $daddr]
    
	set nexthop [$routingTable_ lookup $srcid $dstid]

	set node1 [$self  get-node-by-addr $addr]
	set node2 [$self  get-node-by-addr $nexthop]
    
	set queue_ [[$self link $node1 $node2] queue]
        
	return $queue_
}       
 
Simulator instproc get-link { addr daddr } {
	$self instvar routingTable_

	set srcid [$self get-node-id-by-addr $addr]
	set dstid [$self get-node-id-by-addr $daddr]

	set nexthop [$routingTable_ lookup $srcid $dstid]

	set node1 [$self  get-node-by-addr $addr]
	set node2 [$self  get-node-by-addr $nexthop]

	set link_ [[$self  link $node1 $node2] link]

	return $link_
}
