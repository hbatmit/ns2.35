# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by Sun Microsystems, Inc.
#
# 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
#      promote products derived from this software without specific prior
#      written permission.
#
# SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
# SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
# without express or implied warranty of any kind.
#
# These notices must be retained in any copies of any part of this software.
#
# $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-mip.tcl,v 1.10 2001/02/22 19:45:42 haldar Exp $

# XXX It is stupid to have both MIP implementation here and in mobile node. 
# However, there is no test suite that covers test-suite-mip.tcl, which relies
# on this file, and is implemented using mobile node. WHAT IS GOING ON?? 

#     
# Broadcast Nodes:
# accept limited broadcast packets
#     
#Class Node/Broadcast -superclass Node

Node/Broadcast instproc init {} {
	$self next
        $self instvar address_ classifier_ id_ dmux_

	[Simulator instance] add-broadcast-node $self $id_
	
        set classifier_ [new Classifier/Hash/Dest/Bcast 32]
        $classifier_ set mask_ [AddrParams NodeMask 1]
        $classifier_ set shift_ [AddrParams NodeShift 1]
        set address_ $id_
        if { $dmux_ == "" } {
                set dmux_ [new Classifier/Port/Reserve]
		$dmux_ set mask_ [AddrParams set ALL_BITS_SET]
                $dmux_ set shift_ 0
		$self add-route $address_ $dmux_
		
        }
        $classifier_ bcast-receiver $dmux_

	$self attach-classifier $classifier_
}

Node/Broadcast instproc mk-default-classifier {} { }


# XXX The following three instprocs are used to keep this version of MIP
# work. This is all work-in-progress and everything in this file should 
# be converted into a routing module and merged with mobile node.

#Node/Broadcast instproc sp-add-route { dst target } {
#	[$self set classifier_] install $dst $target
#}

Node/Broadcast instproc add-route { dst target } {
	[$self set classifier_] install $dst $target
}

Node/Broadcast instproc delete-route { dst nullagent } {
	[$self set classifier_] install $dst $nullagent
}

Node/Broadcast instproc add-target { agent port } {
	# Send target
	$agent target [$self entry]
	# Recv target
	[$self demux] install $port $agent
}

MIPEncapsulator instproc tunnel-exit mhaddr {
	$self instvar node_
	return [[$node_ set regagent_] set TunnelExit_($mhaddr)]
}

Class Node/MIPBS -superclass Node/Broadcast

Node/MIPBS instproc init { args } {
	eval $self next $args
	$self instvar regagent_ encap_ decap_ agents_ address_ dmux_ id_

	if { $dmux_ == "" } {
		error "serious internal error at Node/MIPBS\n"
	}
	set regagent_ [new Agent/MIPBS $self]
	$self attach $regagent_ 0
	$regagent_ set mask_ [AddrParams NodeMask 1]
	$regagent_ set shift_ [AddrParams NodeShift 1]
  	$regagent_ set dst_addr_ [expr (~0) << [AddrParams NodeShift 1]]
	$regagent_ set dst_port_ 0

	set encap_ [new MIPEncapsulator]
	set decap_ [new Classifier/Addr/MIPDecapsulator]

	#
	# install en/de-capsulators
	#
	lappend agents_ $decap_
	
	#
	# Check if number of agents exceeds length of port-address-field size
	#
	set nodeaddr [AddrParams addr2id $address_]
	$encap_ set addr_ $nodeaddr
	$encap_ set port_ 1
	$encap_ target [$self entry]
	$encap_ set node_ $self
	
	$dmux_ install 1 $decap_

	$encap_ set mask_ [AddrParams NodeMask 1]
	$encap_ set shift_ [AddrParams NodeShift 1]
	$decap_ set mask_ [AddrParams NodeMask 1]
	$decap_ set shift_ [AddrParams NodeShift 1]
}

Class Node/MIPMH -superclass Node/Broadcast

Node/MIPMH instproc init { args } {
	eval $self next $args
	$self instvar regagent_
	set regagent_ [new Agent/MIPMH $self]
	$self attach $regagent_ 0
	$regagent_ set mask_ [AddrParams NodeMask 1]
	$regagent_ set shift_ [AddrParams NodeShift 1]
 	$regagent_ set dst_addr_ [expr (~0) << [AddrParams NodeShift 1]]
	$regagent_ set dst_port_ 0
}


Agent/MIPBS instproc init { node args } {
	eval $self next $args
    
	# if mobilenode, donot use bcasttarget; use target_ instead;
	if {[$node info class] != "MobileNode/MIPBS" && \
			[$node info class] != "Node/MobileNode"} {
		$self instvar BcastTarget_
		set BcastTarget_ [new Classifier/Replicator]
		$self bcast-target $BcastTarget_
	}
	$self beacon-period 1.0	;# default value
}

Agent/MIPBS instproc clear-reg mhaddr {
	$self instvar node_ OldRoute_ RegTimer_
	if [info exists OldRoute_($mhaddr)] {
		$node_ add-route $mhaddr $OldRoute_($mhaddr)
	}
	if {[$node_ info class] == "MobileNode/MIPBS" || [$node_ info class] =="Node/MobileNode" } {
		eval $node_ delete-route [AddrParams id2addr $mhaddr]
	}
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
		[Simulator instance] cancel $RegTimer_($mhaddr)
		set RegTimer_($mhaddr) ""
	}
}

Agent/MIPBS instproc encap-route { mhaddr coa lifetime } {
	$self instvar node_ TunnelExit_ OldRoute_ RegTimer_
	set ns [Simulator instance]
        set encap [$node_ set encap_]

        if {[$node_ info class] == "MobileNode/MIPBS" || [$node_ info class] == "Node/MobileNode"} {
	    set addr [AddrParams id2addr $mhaddr]
	    set a [split $addr]
	    set b [join $a .]
	    $node_ add-route $b $encap
	} else {
	    set or [[$node_ entry] slot $mhaddr]
	    if { $or != $encap } {
		set OldRoute_($mhaddr) $or
		$node_ add-route $mhaddr $encap
	    }
	}
	set TunnelExit_($mhaddr) $coa
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
		$ns cancel $RegTimer_($mhaddr)
	}
	set RegTimer_($mhaddr) [$ns at [expr [$ns now] + $lifetime] \
				    "$self clear-reg $mhaddr"]
}

Agent/MIPBS instproc decap-route { mhaddr target lifetime } {
	$self instvar node_ RegTimer_
	# decap's for mobilenodes can have a default-target; in this 
	# case the def-target is the routing-agent.
    
	if {[$node_ info class] != "MobileNode/MIPBS" && \
			[$node_ info class] != "Node/MobileNode" } {
		set ns [Simulator instance]
		[$node_ set decap_] install $mhaddr $target
	
		if { [info exists RegTimer_($mhaddr)] && \
				$RegTimer_($mhaddr) != "" } {
			$ns cancel $RegTimer_($mhaddr)
		}
		set RegTimer_($mhaddr) [$ns at [expr [$ns now] + $lifetime] \
				"$self clear-decap $mhaddr"]
	} else {
		[$node_ set decap_] defaulttarget [$node_ set ragent_]
	}
}

Agent/MIPBS instproc clear-decap mhaddr {
	$self instvar node_ RegTimer_
	if { [info exists RegTimer_($mhaddr)] && $RegTimer_($mhaddr) != "" } {
		[Simulator instance] cancel $RegTimer_($mhaddr)
		set RegTimer_($mhaddr) ""
	}
	[$node_ set decap_] clear $mhaddr
}

Agent/MIPBS instproc get-link { src dst } {
	$self instvar node_
	if {[$node_ info class] != "MobileNode/MIPBS" && \
			[$node_ info class] != "Node/MobileNode"} {
		set ns [Simulator instance]
		return [[$ns link [$ns get-node-by-addr $src] \
				[$ns get-node-by-addr $dst]] head]
	} else { 
		return ""
	}
}

Agent/MIPBS instproc add-ads-bcast-link { ll } {
	$self instvar BcastTarget_
	$BcastTarget_ installNext [$ll head]
}

Agent/MIPMH instproc init { node args } {
	eval $self next $args
	# if mobilenode, donot use bcasttarget; use target_ instead;
	if {[$node info class] != "MobileNode/MIPMH" && \
			[$node info class] != "SRNode/MIPMH" && \
			[$node info class] != "Node/MobileNode" } {
		$self instvar BcastTarget_
		set BcastTarget_ [new Classifier/Replicator]
		$self bcast-target $BcastTarget_
	}
	$self beacon-period 1.0	;# default value
}

Agent/MIPMH instproc update-reg coa {
	$self instvar node_
	## dont need to set up routing for mobilenodes, so..
	if {[$node_ info class] != "MobileNode/MIPMH" && \
			[$node_ info class] != "SRNode/MIPMH" && \
			[$node_ info class] != "Node/MobileNode" } {
		set n [Node set nn_]
		set ns [Simulator instance]
		set id [$node_ id]
		set l [[$ns link $node_ [$ns get-node-by-addr $coa]] head]
		for { set i 0 } { $i < $n } { incr i } {
			if { $i != $id } {
				$node_ add-route $i $l
			}
		}
	}
}
    
Agent/MIPMH instproc get-link { src dst } {
	$self instvar node_
	if {[$node_ info class] != "MobileNode/MIPMH" && \
			[$node_ info class] != "SRNode/MIPMH" && \
			[$node_ info class] != "Node/MobileNode" } {
		set ns [Simulator instance]
		return [[$ns link [$ns get-node-by-addr $src] \
			[$ns get-node-by-addr $dst]] head]
	} else {
		return ""
	}
}

Agent/MIPMH instproc add-sol-bcast-link { ll } {
	$self instvar BcastTarget_
	$BcastTarget_ installNext [$ll head]
}






