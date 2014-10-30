#
# abslan.tcl
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

#__________________________________________________________
# AbsLanNode:
# It create an abstract LAN.
# An abstract lan is one in which the complex CSMA/CD contention mechanism
# is replaced by a simple DropTail queue mechanism. 
#____________________________________________________________

#Class AbsLanNode
AbsLanNode set address_   ""

AbsLanNode instproc address  {val} { $self set address_  $val }
AbsLanNode instproc bw       {val} { $self set bw_       $val }
AbsLanNode instproc delay    {val} { $self set delay_    $val }
AbsLanNode instproc qlen     {val} { $self set qlen_     $val }

AbsLanNode instproc init {ns args} {
	set args [eval $self init-vars $args]
	$self instvar bw_ delay_ qlen_
	$self instvar ns_ nodelist_ defRouter_ cost_
	$self instvar id_ address_ q_ dlink_ mcl_ varp_
	$ns instvar Node_

	$self next
	set ns_ $ns
	set nodelist_ ""
	set cost_ 1

	set id_ [Node getid]
	$ns_ add-abslan-node $self $id_
        $self nodeid $id_	;# Propagate id_ into c++ space
	set Node_($id_) $self
        set address_ $id_       ;# won't work for hier rtg!
	set defRouter_ [new LanRouter $ns $self]
	if [$ns multicast?] {
		set switch_ [new Classifier/Hash/Dest 32]
		$switch_ set mask_ [AddrParams set McastMask_]
		$switch_ set shift_ [AddrParams set McastShift_]

		$defRouter_ switch $switch_
	}

	set varp_ [new VARPTable]

	set q_ [new Queue/DropTail]
	set dlink_ [new DelayLink]
	$dlink_ set bandwidth_ $bw_
	$dlink_ set delay_ $delay_
	set mcl_ [new Classifier/Replicator]
	$mcl_ set offset_ [PktHdr_offset PacketHeader/Mac macDA_]
	$mcl_ set direction_ true
	$q_ target $dlink_
	$q_ set limit_ $qlen_
	$dlink_ target $mcl_
	
}

AbsLanNode instproc addNode {nodes} {
	$self instvar id_ lanIface_
	$self instvar q_ ns_ nodelist_ cost_ varp_ 
        $self instvar dlink_ mcl_ bw_
        $self instvar deqT_
	$ns_ instvar link_ Node_ 
         

	set vlinkcost [expr $cost_ / 2.0]
	foreach src $nodes {
		set nif [new AbsLanIface $src $self]
		
		set tr [$ns_ get-ns-traceall]
		if {$tr != ""} {
			$nif trace $ns_ $tr
		}
		
		set tr [$ns_ get-nam-traceall]
		if {$tr != ""} {
			$nif nam-trace $ns_ $tr
		}

						
	        $mcl_ installNext [$nif set mac_]
	        $varp_ mac-addr [[$nif set node_] id] \
			        [[$nif set mac_] id] 
		
		$q_ drop-target [$nif set drophead_]
		set lanIface_($src) $nif

		$src add-neighbor $self
	        
		set sid [$src id]
	       
		set link_($sid:$id_) [new Vlink $ns_ $self $src  $self $bw_ 0]
		set link_($id_:$sid) [new Vlink $ns_ $self $self $src  $bw_ 0]

		#$src add-oif [$link_($sid:$id_) head]  $link_($sid:$id_)
		#$src add-iif [[$nif set iface_] label] $link_($id_:$sid)
		[$link_($sid:$id_) head] set link_ $link_($sid:$id_)

		# Changed to point to common queue
		$link_($sid:$id_) queue [$self set q_ ]
		$link_($id_:$sid) queue [$self set q_ ]

		#$link_($sid:$id_) set iif_ [$nif set iface_]
		#$link_($id_:$sid) set iif_ [$nif set iface_]

		$link_($sid:$id_) cost $vlinkcost
		$link_($id_:$sid) cost $vlinkcost
	}

	set nodelist_ [concat $nodelist_ $nodes]

# Single Deque object ; Not one for each node
# Using the last node as the drc node for create-trace
	set f [$ns_ get-ns-traceall]
	set deqT_ [$ns_ create-trace Deque  $f $src $self ]

	$deqT_ target $dlink_
	$q_ target $deqT_

}

AbsLanNode instproc assign-mac {ip} {
	return $ip ;# use ip addresses at MAC layer
}

AbsLanNode instproc cost c {
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

AbsLanNode instproc cost? {} {
	$self instvar cost_
	return $cost_
}

AbsLanNode instproc rtObject? {} {
	# NOTHING
}

AbsLanNode instproc id {} { $self set id_ }

AbsLanNode instproc node-addr {{addr ""}} { 
	eval $self set address_ $addr
}

AbsLanNode instproc reset {} {
	# NOTHING: needed for node processing by ns routing
}

AbsLanNode instproc is-lan? {} { return 1 }

AbsLanNode instproc dump-namconfig {} {
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

AbsLanNode instproc init-outLink {} { 
	#NOTHING
}

AbsLanNode instproc start-mcast {} { 
	# NOTHING
}

AbsLanNode instproc getArbiter {} {
	# NOTHING
}

AbsLanNode instproc attach {agent} {
	# NOTHING
}

AbsLanNode instproc sp-add-route {args} {
	# NOTHING: use defRouter to find routes
}

AbsLanNode instproc add-route {args} {
	# NOTHING: use defRouter to find routes
}

AbsLanNode instproc add-hroute {args} {
	# NOTHING: use defRouter to find routes
}

AbsLanNode instproc split-addrstr addrstr {
	set L [split $addrstr .]
	return $L
}



#AbsLanIface---------------------------------------------------
#
# node's interface to a AbsLanNode
#------------------------------------------------------------
Class AbsLanIface 

AbsLanIface instproc entry {} { $self set entry_ }

AbsLanIface instproc init {node lan } {

	$self next 

	$self instvar node_ lan_ 
	$self instvar entry_ mac_ ll_ 
        $self instvar drophead_

	set node_ $node
	set lan_ $lan

	set entry_ [new Connector]
        set ll_ [new LL]
        set mac_ [new Mac]
        $mac_ set abstract_ true

	$entry_ target $ll_

        $ll_ mac $mac_
        $ll_ up-target [$node entry]
        $ll_ down-target $mac_
        $ll_ set macDA_ -1
        $ll_ set delay_ 0
        $ll_ lanrouter [$lan set defRouter_]
        $ll_ varp [$lan set varp_]

        $mac_ up-target $ll_
        $mac_ down-target [$lan set q_]
        $mac_ set delay_ 0

        set ns [Simulator instance]
        set drophead_ [new Connector]
        $drophead_ target [$ns set nullAgent_]

        $mac_ drop-target $drophead_
        $ll_ drop-target $drophead_
}

AbsLanIface instproc trace {ns f {op ""}} {
	$self instvar hopT_ rcvT_ enqT_ drpT_ deqT_ 
	$self instvar iface_ entry_ node_ lan_ drophead_ 
	$self instvar ll_ mac_ 

	set hopT_ [$ns create-trace Hop   $f $node_ $lan_  $op]
	set rcvT_ [$ns create-trace Recv  $f $lan_  $node_ $op]
	set enqT_ [$ns create-trace Enque $f $node_ $lan_  $op]
	set drpT_ [$ns create-trace Drop  $f $node_ $lan_  $op]

	$hopT_ target [$entry_ target]
	$entry_ target $hopT_

	$rcvT_ target [$ll_ up-target]
	$ll_ up-target $rcvT_

	$enqT_ target [$mac_ down-target]
	$mac_ down-target $enqT_


	$drpT_ target [$drophead_ target]
	$drophead_ target $drpT_
}

# should be called after LanIface::trace
AbsLanIface instproc nam-trace {ns f} {
	$self instvar hopT_ rcvT_ enqT_  drpT_ deqT_
	if [info exists hopT_] {
		$hopT_ namattach $f
	} else {
		$self trace $ns $f "nam"
	}
	$rcvT_ namattach $f
	$enqT_ namattach $f
	$drpT_ namattach $f
}

#To invoke the creation of an abstract Lan 
Simulator instproc make-abslan {nodelist bw delay {qlen 50}} {
	set lan [new AbsLanNode $self \
			-bw $bw \
			-delay $delay \
			-qlen $qlen]
	$lan addNode $nodelist 
	return $lan
}









