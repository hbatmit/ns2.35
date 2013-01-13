#
# vlan.tcl
#
# Copyright (c) 1997 University of Southern California.
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
# $Header: /cvsroot/nsnam/ns-2/tcl/lan/vlan.tcl,v 1.37 2003/09/11 22:10:20 sfloyd Exp $

# LanNode----------------------------------------------------
#
# Lan implementation as a virtual node: LanNode mimics a real
# node and uses an address (id) from Node's address space
#
# WARNING: if used with hierarchical routing, one has to assign
# a hierarchical address to the lan itself.  This maybe confusing.
#------------------------------------------------------------
#Class LanNode
LanNode set ifqType_   Queue/DropTail
LanNode set ifqLen_    ""
LanNode set llType_    LL
#LanNode set macType_   Mac/Csma/Cd
LanNode set macType_   Mac
LanNode set chanType_  Channel
LanNode set phyType_   Phy/WiredPhy
LanNode set address_   ""
LanNode set mactrace_   false

LanNode instproc address  {val} { $self set address_  $val }
LanNode instproc bw       {val} { $self set bw_       $val }
LanNode instproc delay    {val} { $self set delay_    $val }
LanNode instproc ifqType  {val} { $self set ifqType_  $val }
LanNode instproc ifqLen   {val} { $self set ifqLen_   $val }
LanNode instproc llType   {val} { $self set llType_   $val }
LanNode instproc macType  {val} { $self set macType_  $val }
LanNode instproc chanType {val} { $self set chanType_ $val }
LanNode instproc phyType  {val} { $self set phyType_  $val }
LanNode instproc mactrace    {val} { $self set mactrace_    $val }

LanNode instproc init {ns args} {
	set args [eval $self init-vars $args]
	$self instvar bw_ delay_ llType_ macType_ chanType_
	$self instvar phyType_ mactrace_
	$self instvar ns_ nodelist_ defRouter_ cost_
	$self instvar id_ address_ channel_ mcl_ varp_
	$ns instvar Node_

	$self next
	set ns_ $ns
	set nodelist_ ""
	set cost_ 1

	set id_ [Node getid]
        $self nodeid $id_
        $ns_ add-lannode $self $id_
        set Node_($id_) $self
	if [Simulator hier-addr?] {
		if {$address_ == ""} {
			error "LanNode: use \"-address\" option \
					with hierarchical routing"
		}
	} else {
		set address_ $id_
	}
	$self addr $address_
	set defRouter_ [new LanRouter $ns $self]
	if [$ns multicast?] {
		set switch_ [new Classifier/Hash/Dest 32]
		$switch_ set mask_ [AddrParams McastMask]
		$switch_ set shift_ [AddrParams McastShift]

		$defRouter_ switch $switch_
	}
	set channel_ [new $chanType_]
	set varp_ [new VARPTable]
	#set mcl_ [new Classifier/Mac]
	#$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	#$channel_ target $mcl_
}

LanNode instproc addNode {nodes bw delay {llType ""} {ifqType ""} \
	        {macType ""} {phyType ""} {mactrace ""} {ifqLen ""}} {
	$self instvar ifqType_ ifqLen_ llType_ macType_ chanType_ phyType_ 
	$self instvar mactrace_
	$self instvar id_ channel_ mcl_ lanIface_
	$self instvar ns_ nodelist_ cost_ varp_
	$ns_ instvar link_ Node_ 

	if {$ifqType == ""} { set ifqType $ifqType_ }
	if {$ifqLen == ""} { set ifqLen $ifqLen_ }
	if {$macType == ""} { set macType $macType_ }
	if {$llType  == ""} { set llType $llType_ }
	if {$phyType  == ""} { set phyType $phyType_ }
	if {$mactrace == ""}  { set mactrace $mactrace_ }

	set vlinkcost [expr $cost_ / 2.0]
	foreach src $nodes {
		set nif [new LanIface $src $self \
				-ifqType $ifqType \
				-ifqLen $ifqLen \
				-llType  $llType \
				-macType $macType \
				-phyType $phyType \
				-mactrace $mactrace ]
		
		set tr [$ns_ get-ns-traceall]
		if {$tr != ""} {
			$nif trace $ns_ $tr
		}
		set tr [$ns_ get-nam-traceall]
		if {$tr != ""} {
			$nif nam-trace $ns_ $tr
		}

		
		set ll [$nif set ll_]
		$ll set delay_ $delay
		$ll varp $varp_
		
		$varp_ mac-addr [[$nif set node_] id] \
				[[$nif set mac_] id]
		
		set phy [$nif set phy_]
		$phy node $src
		$phy channel $channel_
		$channel_ addif $phy
		$phy set bandwidth_ $bw

		set lanIface_($src) $nif

		$src add-neighbor $self

		set sid [$src id]
		set link_($sid:$id_) [new Vlink $ns_ $self $src  $self $bw 0]
		set link_($id_:$sid) [new Vlink $ns_ $self $self $src  $bw 0]

		$src add-oif [$link_($sid:$id_) head]  $link_($sid:$id_)
		$src add-iif [[$nif set iface_] label] $link_($id_:$sid)
		[$link_($sid:$id_) head] set link_ $link_($sid:$id_)

		$link_($sid:$id_) queue [$nif set ifq_]
		$link_($id_:$sid) queue [$nif set ifq_]

		$link_($sid:$id_) set iif_ [$nif set iface_]
		$link_($id_:$sid) set iif_ [$nif set iface_]

		$link_($sid:$id_) cost $vlinkcost
		$link_($id_:$sid) cost $vlinkcost
	}
	set nodelist_ [concat $nodelist_ $nodes]
}

LanNode instproc assign-mac {ip} {
	return $ip ;# use ip addresses at MAC layer
}

LanNode instproc cost c {
	$self instvar ns_ nodelist_ id_ cost_
	$ns_ instvar link_
	set cost_ $c
	set vlinkcost [expr $c / 2.0]
	foreach node $nodelist_ {
		set nid [$node id]
		$link_($id_:$nid) cost $vlinkcost
		$link_($nid:$id_) cost $vlinkcost
	}
}

LanNode instproc cost? {} {
	$self instvar cost_
	return $cost_
}

LanNode instproc rtObject? {} {
	# NOTHING
}

LanNode instproc id {} { $self set id_ }

LanNode instproc node-addr {{addr ""}} { 
	eval $self set address_ $addr
}

LanNode instproc reset {} {
	# NOTHING: needed for node processing by ns routing
}

LanNode instproc is-lan? {} { return 1 }

LanNode instproc dump-namconfig {} {
	# Redefine this function if want a different lan layout
	$self instvar ns_ bw_ delay_ nodelist_ id_
	$ns_ puts-nam-config \
			"X -t * -n $id_ -r $bw_ -D $delay_ -o left"
	set cnt 0
	set LanOrient(0) "up"
	set LanOrient(1) "down"
	
	foreach n $nodelist_ {
		$ns_ puts-nam-config \
				"L -t * -s $id_ -d [$n id] -o $LanOrient($cnt)"
		set cnt [expr 1 - $cnt]
	}
}

LanNode instproc init-outLink {} { 
	#NOTHING
}

LanNode instproc start-mcast {} { 
	# NOTHING
}

LanNode instproc getArbiter {} {
	# NOTHING
}

LanNode instproc attach {agent} {
	# NOTHING
}

LanNode instproc sp-add-route {args} {
	# NOTHING: use defRouter to find routes
}

LanNode instproc add-route {args} {
	# NOTHING: use defRouter to find routes
}

LanNode instproc add-hroute {args} {
	# NOTHING: use defRouter to find routes
}

# LanIface---------------------------------------------------
#
# node's interface to a LanNode
#------------------------------------------------------------
Class LanIface 
LanIface set ifqType_ Queue/DropTail
#LanIface set macType_ Mac/Csma/Cd
LanIface set macType_ Mac
LanIface set llType_  LL
LanIface set phyType_  Phy/WiredPhy
LanIface set mactrace_ false

LanIface instproc llType {val} { $self set llType_ $val }
LanIface instproc ifqType {val} { $self set ifqType_ $val }
LanIface instproc ifqLen {val} { $self set ifqLen_ $val }
LanIface instproc macType {val} { $self set macType_ $val }
LanIface instproc phyType {val} { $self set phyType_ $val }
LanIface instproc mactrace {val} { $self set mactrace_ $val }

LanIface instproc entry {} { $self set entry_ }
LanIface instproc init {node lan args} {
	set args [eval $self init-vars $args]
	eval $self next $args

	$self instvar llType_ ifqType_ macType_ phyType_ mactrace_
	$self instvar node_ lan_ ifq_ ifqLen_ mac_ ll_ phy_
	$self instvar iface_ entry_ drophead_

	set node_ $node
	set lan_ $lan

	set ll_ [new $llType_]
	set ifq_ [new $ifqType_]
	if {$ifqLen_ != ""} { $ifq_ set limit_ $ifqLen_ }
	set mac_ [new $macType_]
        if {[string compare $macType_ "Mac/802_3"] == 0} {
	    $mac_ set trace_ $mactrace_
	}
	set iface_ [new NetworkInterface]
	set phy_ [new $phyType_]

	set entry_ [new Connector]
	set drophead_ [new Connector]

	$ll_ set macDA_ -1	;# bcast address if there is no LAN router
	$ll_ lanrouter [$lan set defRouter_]
	$ll_ up-target $iface_
	$ll_ down-target $ifq_
	$ll_ mac $mac_
	$ll_ ifq $ifq_

	$ifq_ target $mac_
	
	$mac_ up-target $ll_
	$mac_ down-target $phy_
	$mac_ netif $phy_
	
	$phy_ up-target $mac_

	$iface_ target [$node entry]
	$entry_ target $ll_

	set ns [Simulator instance]
	
	#drophead is the same for all drops in the lan
	$drophead_ target [$ns set nullAgent_]

	$ifq_ drop-target $drophead_ 
	$mac_ drop-target $drophead_ 
	$ll_ drop-target $drophead_
}

LanIface instproc trace {ns f {op ""}} {
	$self instvar hopT_ rcvT_ enqT_ deqT_ drpT_ 
	$self instvar iface_ entry_ node_ lan_ drophead_ 
	$self instvar ll_ ifq_ mac_ mactrace_

	set hopT_ [$ns create-trace Hop   $f $node_ $lan_  $op]
	set rcvT_ [$ns create-trace Recv  $f $lan_  $node_ $op]
	set enqT_ [$ns create-trace Enque $f $node_ $lan_  $op]
	set deqT_ [$ns create-trace Deque $f $node_ $lan_  $op]
        set drpT_ [$ns create-trace Drop  $f $node_ $lan_  $op]
        if {[string compare $mactrace_ "true"] == 0} {
	    #  For Mac level collision traces 
	    set macdrpT_ [$ns create-trace Collision $f $node_ $lan_ $op]
	    set macdrophead_ [new Connector]
	    $mac_ drop-target $macdrophead_
	    $macdrophead_ target $macdrpT_
	}

	$hopT_ target [$entry_ target]
	$entry_ target $hopT_

	$rcvT_ target [$iface_ target]
	$iface_ target $rcvT_

	$enqT_ target [$ll_ down-target]
	$ll_ down-target $enqT_

	$deqT_ target [$ifq_ target]
	$ifq_ target $deqT_

	$drpT_ target [$drophead_ target]
	$drophead_ target $drpT_
}
# should be called after LanIface::trace
LanIface instproc nam-trace {ns f} {
	$self instvar hopT_ rcvT_ enqT_ deqT_ drpT_ 
	if [info exists hopT_] {
		$hopT_ namattach $f
	} else {
		$self trace $ns $f "nam"
	}
	$rcvT_ namattach $f
	$enqT_ namattach $f
	$deqT_ namattach $f
	$drpT_ namattach $f
}
LanIface instproc add-receive-filter filter {
	$self instvar mac_
	$filter target [$mac_ target]
	$mac_ target $filter
}


# Vlink------------------------------------------------------
#
# Virtual link  implementation. Mimics a Simple Link but with
# zero delay and infinite bandwidth
#------------------------------------------------------------
Class Vlink
Vlink instproc up? {} {
	return "up"
}
Vlink instproc queue {{q ""}} {
	eval $self set queue_ $q
}
Vlink instproc init {ns lan src dst b d} {
	$self instvar ns_ lan_ src_ dst_ bw_ delay_

	set ns_ $ns
	set lan_ $lan
	set src_ $src
	set dst_ $dst
	set bw_ $b
	set delay_ $d
}
Vlink instproc src {}	{ $self set src_	}
Vlink instproc dst {}	{ $self set dst_	}
Vlink instproc dump-nam-queueconfig {} {
	#NOTHING
}
Vlink instproc head {} {
	$self instvar lan_ dst_ src_
	if {$src_ == $lan_ } {
		# if this is a link FROM the lan vnode, 
		# it doesn't matter what we return, because
		# it's only used by $lan add-route (empty)
		return ""
	} else {
		# if this is a link TO the lan vnode, 
		# return the entry to the lanIface object
		set src_lif [$lan_ set lanIface_($src_)]
		return [$src_lif entry]
	}
}
Vlink instproc cost c { $self set cost_ $c}	
Vlink instproc cost? {} {
	$self instvar cost_
	if ![info exists cost_] {
		return 1
	}
	return $cost_
}


# LanRouter--------------------------------------------------
#
# "Virtual node lan" needs to know which of the lan nodes is
# the next hop towards the packet's (maybe remote) destination.
#------------------------------------------------------------
LanRouter instproc init {ns lan} {
	$self next
	if [Simulator hier-addr?] {
		$self routing hier
	} else {
		$self routing flat
	}
	$self lanaddr [$lan node-addr]
	$self routelogic [$ns get-routelogic]
}


Node instproc is-lan? {} { return 0 }

#
# newLan:  create a LAN from a sete of nodes
#
Simulator instproc newLan {nodelist bw delay args} {
	set lan [eval new LanNode $self -bw $bw -delay $delay $args]
	$lan addNode $nodelist $bw $delay
	return $lan
}


# For convenience, use make-lan.  For more fine-grained control,
# use newLan instead of make-lan.
#{macType Mac/Csma/Cd} -> for now support for only Mac
Simulator instproc make-lan { args } {
       
        set t [lindex $args 0]
        set mactrace "false"
        if { $t == "-trace" } {
	    set mactrace [lindex $args 1]
	    if {$mactrace == "on" } {
		set mactrace "true"
	    }
	
	}
	
	if { $t == "-trace" } {
	    set nodelist [lindex $args 2]
	    set bw [lindex $args 3]
	    set delay [lindex $args 4]
	    set llType [lindex $args 5]
	    set ifqType [lindex $args 6]
	    set macType [lindex $args 7]
	    set chanType [lindex $args 8]
	    set phyType [lindex $args 9]
	    set ifqLen [lindex $args 10]
	} else {
	    set nodelist [lindex $args 0]
	    set bw [lindex $args 1]
	    set delay [lindex $args 2]
	    set llType [lindex $args 3]
	    set ifqType [lindex $args 4]
	    set macType [lindex $args 5]
	    set chanType [lindex $args 6]
	    set phyType [lindex $args 7]
	    set ifqLen [lindex $args 8]
	}
	
	if { $llType == "" } {
	    set llType "LL"
	}
	if { $ifqType == "" } {
	    set ifqtype "Queue/DropTail"
	}
	if { $macType == "" } {
	    set macType "Mac"
	}
	if { $chanType == "" } {
	    set chanType "Channel"
	}
	if { $phyType == ""} {
	    set phyType "Phy/WiredPhy"
	}
    
	if {[string compare $macType "Mac/Csma/Cd"] == 0} {
	    puts "Warning: Mac/Csma/Cd is out of date"
	    puts "Warning: Please use Mac/802_3 to replace Mac/Csma/Cd"
	    set macType "Mac/802_3"
	}

	set lan [new LanNode $self \
			-bw $bw \
			-delay $delay \
			-llType $llType \
			-ifqType $ifqType \
			-ifqLen $ifqLen \
			-macType $macType \
			-chanType $chanType \
			-phyType $phyType \
			-mactrace $mactrace]
	$lan addNode $nodelist $bw $delay $llType $ifqType $macType \
			$phyType $mactrace $ifqLen
	
	return $lan
}










