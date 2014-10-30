# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-22 09:41:04 haoboy>
#
# Copyright (c) 1996-2000 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-node.tcl,v 1.89 2001/11/06 06:16:21 tomh Exp $
#

Node set nn_ 0
Node proc getid {} {
	set id [Node set nn_]
	Node set nn_ [expr $id + 1]
	return $id
}

# The list of modules to be installed. More can be added via node-config. 
# Base should always be the first module in this list.
Node set module_list_ { Base }

Node proc enable-module { mod_name } {
	Node instvar module_list_
	if { [lsearch $module_list_ $mod_name] < 0 } {
		lappend module_list_ $mod_name
	}
}

Node proc disable-module { mod_name } {
	Node instvar module_list_
	set pos [lsearch $module_list_ $mod_name]
	if { $pos >= 0 } {
		set module_list_ [lreplace $module_list_ $pos $pos]
	}
}

Node instproc init args {
	eval $self next $args

        $self instvar id_ agents_ dmux_ neighbor_ rtsize_ address_ \
			nodetype_ multiPath_ ns_ rtnotif_ ptnotif_

	set ns_ [Simulator instance]
	set id_ [Node getid]
	$self nodeid $id_	;# Propagate id_ into c++ space

	if {[llength $args] != 0} {
		set address_ [lindex $args 0]
	} else {
		set address_ $id_
	}
	$self cmd addr $address_; # Propagate address_ into C++ space
	#$ns_ add-node $self $id_        
	set neighbor_ ""
	set agents_ ""
	set dmux_ ""
        set rtsize_ 0
	set ptnotif_ {}
	set rtnotif_ {}
	set nodetype_ [$ns_ get-nodetype]

	$self mk-default-classifier

	# XXX Eventually these two should also be converted to modules
	set multiPath_ [$class set multiPath_]
}

# XXX This instproc is backward compatibility; when satellite node, mobile
# node (incl. MIP) are converted to routing modules, this should be merged 
# into init{}
Node instproc mk-default-classifier {} {
	Node instvar module_list_
	# At minimum we should enable base module
	foreach modname [Node set module_list_] {
		$self register-module [new RtModule/$modname]
	}
}

Node instproc id {} {
	return [$self set id_]
}

Node instproc node-addr {} {
	return [$self set address_]
}

#----------------------------------------------------------------------
# XXX This should eventually go away, as we replace nodetype_ with 
# routing modules
Node instproc node-type {} {
	return [$self set nodetype_]
}
#----------------------------------------------------------------------


#
# Module registration
#
Node instproc register-module { mod } {
	$self instvar reg_module_
	$mod register $self
	set reg_module_([$mod module-name]) $mod
}

Node instproc unregister-module { mod } {
	$self instvar reg_module_
	$mod unregister
	# We do not explicitly check the existence of reg_module_($name).
	# If not, tcl will issue an error by itself. 
	unset reg_module_([$mod module-name])
	delete $mod
}

Node instproc list-modules {} {
	$self instvar reg_module_
	set ret ""
	foreach n [array names reg_module_] {
		lappend ret $reg_module_($n)
	}
	return $ret
}

Node instproc get-module { name } {
	$self instvar reg_module_
	# Must do an error check here.
	if [info exists reg_module_($name)] {
		return $reg_module_($name)
	} else {
		return ""
	}
}


#
# Address classifier manipulation
#

# Increase the routing table size counter - keeps track of rtg table 
# size for each node. For bookkeeping only.
Node instproc incr-rtgtable-size {} {
	$self instvar rtsize_
	incr rtsize_
}

Node instproc decr-rtgtable-size {} {
	$self instvar rtsize_
	incr rtsize_ -1
}

Node instproc entry {} {
	return [$self set classifier_]
}

# Place classifier $clsfr at the node entry point. 
#
# - Place the existing entry at slot $hook of the new classifier. Usually 
#   it is an address classifier sitting in a node entry, so this means that
#   a special address should be assigned to pass packets to the original 
#   classifier, if this is necessary. 
#
# - Associate the new classifier with $module (stored in mod_assoc_). This
#   information is used later when the associated classifier is replaced, 
#   then $module will be notified of this event and requested to unregister.
#
# - The original classifier is stored in hook_assoc_, this is used in 
#   install-entry{} to connect the new classifier to the "classifier chain"
#   to which the replaced classifier is linked.
#
# - To install at the entry something derived from Connector rather than 
#   Classifier, you should specify $hook as "target". 
Node instproc insert-entry { module clsfr {hook ""} } {
	$self instvar classifier_ mod_assoc_ hook_assoc_
	if { $hook != "" } {
		# Build a classifier "chain" when specified
		set hook_assoc_($clsfr) $classifier_
		if { $hook == "target" } {
			$clsfr target $classifier_
		} elseif { $hook != "" } {
			$clsfr install $hook $classifier_
		}
	}
	# Associate this module to the classifier, so if the classifier is
	# removed later, we'll remove the module as well.
	set mod_assoc_($clsfr) $module
	set classifier_ $clsfr
}

# We separate insert-entry from install-entry because we need a method to 
# replace the current entry classifier and _remove_ routing functionality 
# associated with that classifier. These two methods cannot be merged by 
# checking whether $hook is empty. 
Node instproc install-entry { module clsfr {hook ""} } {
	$self instvar classifier_ mod_assoc_ hook_assoc_
	if [info exists classifier_] {
		if [info exists mod_assoc_($classifier_)] {
			$self unregister-module $mod_assoc_($classifier_)
			unset mod_assoc_($classifier_)
		}
		# Connect the new classifier to the existing classifier chain,
		# if there is any.
		if [info exists hook_assoc_($classifier_)] {
			if { $hook == "target" } {
				$clsfr target $hook_assoc($classifier_)
			} elseif { $hook != "" } {
				$clsfr install $hook $hook_assoc_($classifier_)
			}
			set hook_assoc_($clsfr) $hook_assoc_($classifier_)
			unset hook_assoc_($classifier_)
		}
	}
	set mod_assoc_($clsfr) $module
	set classifier_ $clsfr
}

# CHANGE:
# Node no longer keeps list of all rtmodules to be notified.
# Instead we now have a chain (linked list) of modules, 
# with rtnotif_ being the head of this linked list. 
# Routing info flows from thru all rtmodules linked by this 
# chain.

# Whenever a route is added or deleted, $module should be notified
Node instproc route-notify { module } {
	$self instvar rtnotif_
	if {$rtnotif_ == ""} {
		set rtnotif_ $module
	} else {
		$rtnotif_ route-notify $module
	}

	$module cmd route-notify $self
}

Node instproc unreg-route-notify { module } {
	$self instvar rtnotif_
	if {$rtnotif_ != ""} {
		if {$rtnotif_ == $module} {
			set rtnotif_ [$rtnotif_ set next_rtm_]
		} else {
			$rtnotif_ unreg-route-notify $module
		}
	}

	$module cmd unreg-route-notify $self
}

Node instproc add-route { dst target } {
	$self instvar rtnotif_
	# Notify every module that is interested about this 
	# route installation
	
	if {$rtnotif_ != ""} {
		$rtnotif_ add-route $dst $target
	}
	$self incr-rtgtable-size
}

Node instproc delete-route args {
	$self instvar rtnotif_
	if {$rtnotif_ != ""} {
		eval $rtnotif_ delete-route $args
	}
	$self decr-rtgtable-size
}

#
# Node support for detailed dynamic unicast routing
#
Node instproc init-routing rtObject {
	$self instvar multiPath_ routes_ rtObject_
	set nn [$class set nn_]
	for {set i 0} {$i < $nn} {incr i} {
		set routes_($i) 0
	}
	if ![info exists rtObject_] {
		$self set rtObject_ $rtObject
	}
	$self set rtObject_
}

Node instproc rtObject? {} {
    $self instvar rtObject_
	if ![info exists rtObject_] {
		return ""
	} else {
		return $rtObject_
    }
}

Node instproc intf-changed {} {
	$self instvar rtObject_
	if [info exists rtObject_] {	;# i.e. detailed dynamic routing
		$rtObject_ intf-changed
	}
}


#----------------------------------------------------------------------

# XXX Eventually add-routes{} and delete-routes{} should be 
# unified with add-route{} for a single interface to install routes.

# Node support for equal cost multi path routing
Node instproc add-routes {id ifs} {
	$self instvar classifier_ multiPath_ routes_ mpathClsfr_
	if !$multiPath_ {
		if {[llength $ifs] > 1} {
			warn "$class::$proc cannot install multiple routes"
			set ifs [lindex $ifs 0]
		}
		$self add-route $id [$ifs head]
		set routes_($id) 1
		return
	}
	if {$routes_($id) <= 0 && [llength $ifs] == 1 && \
			![info exists mpathClsfr_($id)]} {
		$self add-route $id [$ifs head]
		set routes_($id) 1
	} else {
		if ![info exists mpathClsfr_($id)] {
			#
			# 1. get new MultiPathClassifier,
			# 2. migrate existing routes to that mclassifier
			# 3. install the mclassifier in the node classifier_
			#
			set mpathClsfr_($id) [new Classifier/MultiPath]
			if {$routes_($id) > 0} {
				assert "$routes_($id) == 1"
				$mpathClsfr_($id) installNext \
						[$classifier_ in-slot? $id]
			}
			$classifier_ install $id $mpathClsfr_($id)
		}
		foreach L $ifs {
			$mpathClsfr_($id) installNext [$L head]
			incr routes_($id)
		}
	}
}

Node instproc delete-routes {id ifs nullagent} {
	$self instvar mpathClsfr_ routes_
	if [info exists mpathClsfr_($id)] {
		foreach L $ifs {
			set nonLink([$L head]) 1
		}
		foreach {slot link} [$mpathClsfr_($id) adjacents] {
			if [info exists nonLink($link)] {
				$mpathClsfr_($id) clear $slot
				incr routes_($id) -1
			}
		}
	} else {
		$self delete-route $id $nullagent
		incr routes_($id) -1
		# Notice that after this operation routes_($id) may not 
		# necessarily be 0.
	}
}

# Enable multicast routing support in this node. 
# 
# XXX This instproc should ONLY be used when you want multicast only on a 
# subset of nodes as a means to save memory. If you want to run multicast on
# your entire topology, use [new Simulator -multicast on].
Node instproc enable-mcast args {
	# Do NOT add Mcast using Node enable-module, because the latter 
	# enables mcast for all nodes that may be created later
	$self register-module [new RtModule/Mcast]
	
}

#--------------------------------------------------------------

#
# Port classifier manipulation
#

Node instproc alloc-port { nullagent } {
	return [[$self set dmux_] alloc-port $nullagent]
}

Node instproc agent port {
	$self instvar agents_
	foreach a $agents_ {
		if { [$a set agent_port_] == $port } {
			return $a
		}
	}
	return ""
}

Node instproc demux {} {
	return [$self set dmux_]
}

# Install $demux as the default demuxer of the node, place the existing demux
# at $port of the new demuxer. 
#
# XXX Here we do not have a similar difference like that between 
# "insert-entry" and "install-entry", because even if a demux 
# is replaced, the associated routing module should not be removed. 
#
# This is a rather arbitrary decision, but we do not have better clue
# how demuxers will be used among routing modules.
Node instproc install-demux { demux {port ""} } {
	$self instvar dmux_ address_
	if { $dmux_ != "" } {
		$self delete-route $dmux_
		if { $port != "" } {
			$demux install $port $dmux_
		}
	}
	set dmux_ $demux
	$self add-route $address_ $dmux_
}

# Whenever an agent is attached or detached, $module should be notified.
Node instproc port-notify { module } {
	$self instvar ptnotif_
	lappend ptnotif_ $module
}

Node instproc unreg-port-notify { module } {
	$self instvar ptnotif_
	set pos [lsearch $ptnotif_ $module]
	if { $pos >= 0 } {
		set ptnotif_ [lreplace $ptnotif_ $pos $pos]
	}
}

#
# Attach an agent to a node: pick a port and bind the agent to the port.
#
# To install module-specific demuxers, do that in your module's register{}
# method. Since this method will be called during Node::init{}, when 
# Node::attach{} is called dmux_ will already be present hence the following
# default dmux_ construction will not be triggered.
#
Node instproc attach { agent { port "" } } {
	$self instvar agents_ address_ dmux_ 
	#
	# Assign port number (i.e., this agent receives
	# traffic addressed to this host and port)
	#
	lappend agents_ $agent
	#
	# Attach agents to this node (i.e., the classifier inside).
	# We call the entry method on ourself to find the front door
	# (e.g., for multicast the first object is not the unicast classifier)
	#
	# Also, stash the node in the agent and set the local addr of 
	# this agent.
	#
	$agent set node_ $self
	$agent set agent_addr_ [AddrParams addr2id $address_]
	#
	# If a port demuxer doesn't exist, create it.
	#
	if { $dmux_ == "" } {
		# Use the default mask_ and port_ values
		set dmux_ [new Classifier/Port]
		# point the node's routing entry to itself
		# at the port demuxer (if there is one)
		$self add-route $address_ $dmux_
	}
	if { $port == "" } {
		set port [$dmux_ alloc-port [[Simulator instance] nullagent]]
	}
	$agent set agent_port_ $port

	$self add-target $agent $port
}

# XXX For backward compatibility. When both satellite node and mobile node
# are converted to modules, this should be merged into attach{}.
Node instproc add-target { agent port } {
	$self instvar ptnotif_
	# Replaces the following line from old ns (2.1b7 and earlier)
	#   $self add-target $agent $port
	foreach m [$self set ptnotif_] {
		$m attach $agent $port
	}
}

# Detach an agent from a node.
Node instproc detach { agent nullagent } {
	$self instvar agents_ dmux_
	#
	# remove agent from list
	#
	set k [lsearch -exact $agents_ $agent]
	if { $k >= 0 } {
		set agents_ [lreplace $agents_ $k $k]
	}
	#
	# sanity -- clear out any potential linkage
	#
	$agent set node_ ""
	$agent set agent_addr_ 0
	$agent target $nullagent
	# Install nullagent to sink transit packets   
	$dmux_ install [$agent set agent_port_] $nullagent

	foreach m [$self set ptnotif_] {
		$m detach $agent $nullagent
	}
}

# reset all agents attached to this node
Node instproc reset {} {
	$self instvar agents_
	foreach a $agents_ {
		$a reset
	}
	foreach m [$self list-modules] {
		$m reset
	}
}


#
# Some helpers
#
Node instproc neighbors {} {
	$self instvar neighbor_
	return [lsort $neighbor_]
}

Node instproc add-neighbor {p {pushback 0}} {
	$self instvar neighbor_
	lappend neighbor_ $p
	
	#added for keeping the neighbor list in the Node (for pushback) - ratul
	if { $pushback == 1 } {
		$self cmd add-neighbor $p
	}
}

Node instproc is-neighbor { node } {
	$self instvar neighbor_
	return [expr [lsearch $neighbor_ $node] != -1]
}
