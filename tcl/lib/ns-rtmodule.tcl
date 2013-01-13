# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
#  Time-stamp: <2000-09-13 18:22:04 haoboy>
# 
#  Copyright (c) 2000 by the University of Southern California
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
#  $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-rtmodule.tcl,v 1.13 2005/09/16 03:05:43 tomh Exp $
#
# OTcl interface definition for the base routing module. They provide 
# linkage to Node, hence all derived classes should inherit these interfaces
# and fill in their specific handling code.

RtModule instproc register { node } {
	# Attach to node and register routing notifications
	$self attach-node $node
	$node route-notify $self
	$node port-notify $self
}

RtModule instproc init {} {
	$self next
	$self instvar classifier_ next_rtm_
	set next_rtm_ ""
	set classifier_ ""
}

# Only called when the default classifier of this module is REPLACED.
RtModule instproc unregister {} {
	$self instvar classifier_
	delete $classifier_
	[$self node] unreg-route-notify $self
	[$self node] unreg-port-notify $self
}

RtModule instproc route-notify { module } {
	$self instvar next_rtm_
	if {$next_rtm_ == ""} {
		set next_rtm_ $module
	} else {
		$next_rtm_ route-notify $module
	}
}

RtModule instproc unreg-route-notify { module } {
	$self instvar next_rtm_
	if {$next_rtm_ != ""} {
		if {$next_rtm_ == $module} {
			set next_rtm_ [$next_rtm_ set next_rtm_]
		} else {
			$next_rtm_ unreg-route-notify $module
		}
	}
}

RtModule instproc add-route { dst target } {
	$self instvar next_rtm_
	[$self set classifier_] install $dst $target
	if {$next_rtm_ != ""} {
		$next_rtm_ add-route $dst $target
	}
}

RtModule instproc delete-route { dst nullagent} {
	$self instvar next_rtm_
	[$self set classifier_] install $dst $nullagent
	if {$next_rtm_ != ""} {
		$next_rtm_ delete-route $dst $nullagent
	}
}

RtModule instproc attach { agent port } {
	# Send target
	$agent target [[$self node] entry]
	# Recv target
	[[$self node] demux] install $port $agent
}

RtModule instproc detach { agent nullagent } {
	# Empty by default
}

RtModule instproc reset {} {
	# Empty by default
}

#
# Base routing module
#

# Use standard naming although this is a pure OTcl class 
#
# In fact, all functionalities of Base are already implemented in RtModule.
# We add this class only to provide a uniform interface to the RtModule 
# creation process, where a single line [new RtModule/$name] is used.
#
# Defined in ~ns/rtmodule.{h,cc}

RtModule/Base instproc register { node } {
	$self next $node

	$self instvar classifier_
	set classifier_ [new Classifier/Hash/Dest 32]
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	# XXX Base should ALWAYS be the first module to be installed.

	$node install-entry $self $classifier_
}


#
# Illustrates usage of insert-entry{}. However:
# 
# XXX Ugly hack: We should keep all variables local to this particular
# module. However, given all existing multicast code assumes that 
# switch_ and multiclassifier_ are inside Node, we have to make this 
# compromise if we do not want to modify all existing code. :(
#
RtModule/Mcast instproc register { node } {
	$self next $node
	$self instvar classifier_
	
	# Keep old classifier so we can use RtModule::add-route{}.
	$self set classifier_ [$node entry]
	
	#if {[$classifier_ info class] != "Classifier/Virtual"} {
	# donot want to add-route if virtual classifier
	#$self attach-classifier $classifier_
	#}
	
	$node set switch_ [new Classifier/Addr]

	# Set up switch to route unicast packet to slot 0 and
	# multicast packets to slot 1
	[$node set switch_] set mask_ [AddrParams McastMask]
	[$node set switch_] set shift_ [AddrParams McastShift]

	# Create a classifier for multicast routing
	$node set multiclassifier_ [new Classifier/Multicast/Replicator]
	[$node set multiclassifier_] set node_ $node
	
	$node set mrtObject_ [new mrtObject $node]

	# Install existing classifier at slot 0, new classifier at slot 1
	$node insert-entry $self [$node set switch_] 0
	[$node set switch_] install 1 [$node set multiclassifier_]
}

#
# Hierarchical routing module. 
#
RtModule/Hier instproc register { node } {
	$self next $node
	$self instvar classifier_
	set classifier_ [new Classifier/Hier]
	$node install-entry $self $classifier_
}

RtModule/Hier instproc delete-route args {
	eval [$self set classifier_] clear $args
}

Classifier/Hier instproc init {} {
	$self next
	for {set n 1} {$n <= [AddrParams hlevel]} {incr n} {
		set classifier [new Classifier/Addr]
		$classifier set mask_ [AddrParams NodeMask $n]
		$classifier set shift_ [AddrParams NodeShift $n]
		$self cmd add-classifier $n $classifier
	}
}

Classifier/Hier instproc destroy {} {
	for {set n 1} {$n <= [AddrParams hlevel]} {incr n} {
		delete [$self cmd classifier $n]
	}
	$self next
}

Classifier/Hier instproc clear args {
	set l [llength $args]
	[$self cmd classifier $l] clear [lindex $args [expr $l-1]] 
}

Classifier/Hier instproc install { dst target } {
	set al [AddrParams split-addrstr $dst]
	set l [llength $al]
	for {set i 1} {$i < $l} {incr i} {
		set d [lindex $al [expr $i-1]]
		[$self cmd classifier $i] install $d \
				[$self cmd classifier [expr $i+1]]
	}
	[$self cmd classifier $l] install [lindex $al [expr $l-1]] $target
}


#
# Manual Routing Nodes:
# like normal nodes, but with a hash classifier.
#
RtModule/Manual instproc register { node } {
	$self next $node
	$self instvar classifier_	
	# Note the very small hash size---
	# you're supposed to resize it if you want more.
	set classifier_ [new Classifier/Hash/Dest 2]
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	$node install-entry $self $classifier_
}

RtModule/Manual instproc add-route {dst_address target} {
	$self instvar classifier_ 
	set slot [$classifier_ installNext $target]
	if {$dst_address == "default"} {
		$classifier_ set default_ $slot
	} else {
		# don't encode the address here, set-hash bypasses that for us
		set encoded_dst_address [expr $dst_address << [AddrParams NodeShift 1]]
		$classifier_ set-hash auto 0 $encoded_dst_address 0 $slot
	}
}

RtModule/Manual instproc add-route-to-adj-node { args } {
	$self instvar classifier_ 

	set dst ""
	if {[lindex $args 0] == "-default"} {
		set dst default
		set args [lrange $args 1 end]
	}
	if {[llength $args] != 1} {
		error "ManualRtNode::add-route-to-adj-node [-default] node"
	}
	set target_node $args
	if {$dst == ""} {
		set dst [$target_node set address_]
	}
	set ns [Simulator instance]
	set link [$ns link [$self node] $target_node]
	set target [$link head]
	return [$self add-route $dst $target]
}

#
# Source Routing Nodes.
#
RtModule/Source instproc register { node } {
        $self next $node

        $self instvar classifier_
        # Keep old classifier so we can use RtModule::add-route{}.
        $self set classifier_ [$node entry]

        # Set up switch to route unicast packet to slot 0 and
        # multicast packets to slot 1
        #[$node set switch_] set mask_ [AddrParams McastMask]
        #[$node set switch_] set shift_ [AddrParams McastShift]

        # Create a classifier for multicast routing
        $node set src_classifier_ [new Classifier/SR]
        $node set src_agent_ [new Agent/SRAgent]
        $node set switch_ [$node set src_classifier_]

        # $node set multiclassifier_ [new Classifier/Multicast/Replicator]
        # [$node set multiclassifier_] set node_ $node



#       $node set mrtObject_ [new mrtObject $node]

        # Install existing classifier at slot 0, new classifier at slot 1
        $node insert-entry $self [$node set switch_] 1

        [$node set switch_]  install 0 [$node set src_agent_]
        $node attach [$node set src_agent_]

#       $self set src_rt 1

}


#
# Virtual Classifier Nodes:
# like normal nodes, but with a virtual unicast classifier.
#
RtModule/VC instproc register { node } {
	# We do not do route-notify. Only port-notify will suffice.
	$self instvar classifier_

	$self attach-node $node
	$node port-notify $self

	set classifier_ [new Classifier/Virtual]
	$classifier_ set node_ $node
	$classifier_ set mask_ [AddrParams NodeMask 1]
	$classifier_ set shift_ [AddrParams NodeShift 1]
	$classifier_ nodeaddr [$node node-addr]
	$node install-entry $self $classifier_ 
}

RtModule/VC instproc add-route { dst target } {
}

Classifier/Virtual instproc find dst {
	$self instvar node_
	if {[$node_ id] == $dst} {
		return [$node_ set dmux_]
	} else {
		return [[[Simulator instance] link $node_ \
				[[Simulator instance] set Node_($dst)]] head]
	}
}

Classifier/Virtual instproc install {dst target} {
}

