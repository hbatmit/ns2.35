#
# tcl/ctr-mcast/CtrMcast.tcl
#
# Copyright (C) 1997 by USC/ISI
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
# Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
########## CtrMcast Class: Individual Node join-group, leave-group, etc #####
Class CtrMcast -superclass McastProtocol

CtrMcast instproc init { sim node } {
	$self next $sim $node
	$self instvar ns_ node_
	$self instvar agent_ defaultTree_ decapagent_
	$self instvar c_rp_ c_bsr_ priority_

	set agent_ [$ns_ set MrtHandle_]

	set defaultTree_ "RPT"

	set decapagent_ [new Agent/Decapsulator]
	$ns_ attach-agent $node_ $decapagent_

	### config PIM nodes
	set c_rp_      1
	set c_bsr_     1
	set priority_  0
}

CtrMcast instproc join-group  { group } {
	$self next $group
	$self instvar node_ ns_ agent_
	$self instvar defaultTree_

	if { [$agent_ treetype? $group] == "" } {
		$agent_ treetype $group $defaultTree_
		$agent_ add-new-group $group
	}

	$agent_ add-new-member $group $node_

	foreach src [$agent_ sources? $group] {
		$agent_ compute-branch $src $group $node_
	}
}

CtrMcast instproc leave-group  { group } {
	$self next $group
	$self instvar node_ ns_ agent_ defaultTree_

	$agent_ remove-member $group $node_
	foreach src [$agent_ sources? $group] {
		$agent_ prune-branch $src $group $node_
	}
}

CtrMcast instproc handle-cache-miss { srcID group iface } {
	$self instvar ns_ agent_ node_
	$self instvar defaultTree_

	if { [$agent_ treetype? $group] == "" } {
		$agent_ treetype $group $defaultTree_
		#$agent_ add-new-member $group $node_
	}
	if { [$node_ id] == $srcID } {
		set RP [$self get_rp $group]
		if {[$agent_ treetype? $group] == "RPT" && $srcID != [$RP id]} {
			set encapagent [new Agent/Encapsulator]
			$ns_ attach-agent $node_ $encapagent

			set ctrmcast [[$RP getArbiter] getType "CtrMcast"]
			$ns_ connect $encapagent [$ctrmcast set decapagent_]

			### create (S,G,iif=-1) entry
			$node_ add-mfc-reg $srcID $group -1 $encapagent
		}
		
		if [$agent_ new-source? $group $node_] {
			$agent_ compute-tree $node_ $group
		}
	} elseif [SessionSim set MixMode_] {
	    set srcnode [$ns_ get-node-by-id $srcID]
	    if [$agent_ new-source? $group $srcnode] {
		$agent_ compute-tree $srcnode $group
	    }
	}
	return 1 ;#call again
}

CtrMcast instproc drop  { replicator src group iface } {
	#packets got dropped only due to null oiflist
}

CtrMcast instproc handle-wrong-iif { srcID group iface } {
	warn "$self: $proc for <S: $srcID, G: $group, if: $iface>?"
	return 0 ;#call once
}

CtrMcast instproc notify { dummy } {
}
##### Two functions to help get RP for a group #####
##### get_rp {group}                            #####
##### hash {rp group}                          #####
CtrMcast instproc get_rp group {
	$self instvar rpset_ agent_

	if ![info exists rpset_] {
		[$agent_ set ctrrpcomp] compute-rpset
		assert [info exists rpset_]
	}
	set returnrp -1
	set hashval -1
	foreach rp $rpset_ {
	        if {[$self hash $rp $group] > $hashval} {
		        set hashval [$self hash $rp $group]
		        set returnrp $rp
		}
	}
	set returnrp		;# return
}

CtrMcast instproc hash {rp group} {
	$rp id
}

CtrMcast instproc set-rpset args {
	eval $self set rpset_ "$args"
}

CtrMcast instproc get_bsr {} {
	warn "$self: CtrMcast doesn't require a BSR"
}

CtrMcast instproc set_c_bsr { prior } {
	$self instvar c_bsr_ priority_
	set c_bsr_ 1
	set priority_ $prior
}

CtrMcast instproc set_c_rp {} {
	$self instvar c_rp_
	set c_rp_ 1
}

CtrMcast instproc unset_c_rp {} {
	$self instvar c_rp_
	set c_rp_ 0
}

##################### MultiNode: add-mfc-reg ################

Node instproc add-mfc-reg { src group iif oiflist } {
	$self instvar multiclassifier_ Regreplicator_

	#XXX node addr is in upper 24 bits

	if [info exists Regreplicator_($group)] {
		foreach oif $oiflist {
			$Regreplicator_($group) insert $oif
		}
		return 1
	}
	set r [new Classifier/Replicator/Demuxer]
	$r set node_ $self
	$r set srcID_ $src
	set Regreplicator_($group) $r

	foreach oif $oiflist {
		$r insert $oif
	}

	# Install the replicator.  We do this only once and leave
	# it forever.  Since prunes are data driven, we want to
	# leave the replicator in place even when it's empty since
	# the replicator::drop callback triggers the prune.
	#
	$multiclassifier_ add-rep $r $src $group $iif
}

Node instproc getRegreplicator group {
	$self instvar Regreplicator_
	if [info exists Regreplicator_($group)] {
		return $Regreplicator_($group)
	} else {
		return -1
	}
}
