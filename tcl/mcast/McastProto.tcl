#
# tcl/mcast/McastProto.tcl
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
# Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
# 
#
Class McastProtocol

McastProtocol instproc init {sim node} {
	$self next
	$self instvar ns_ node_ status_ type_ id_
	set ns_   $sim
	set node_ $node
	set status_ "down"
	set type_   [$self info class]
	set id_ [$node id]

	$ns_ maybeEnableTraceAll $self $node_
}

McastProtocol instproc getType {} { $self set type_ }

McastProtocol instproc start {}		{ $self set status_ "up"   }
McastProtocol instproc stop {}		{ $self set status_ "down" }
McastProtocol instproc getStatus {}	{ $self set status_	   }

McastProtocol instproc upcall {code args} {
	# currently expects to handle cache-miss and wrong-iif
	eval $self handle-$code $args
}
 
McastProtocol instproc handle-wrong-iif { srcID group iface } {
	# return values: 
	#   0 : do not call classify on this packet again
	#   1 : changed iif for the corresponding mfc-entry, classify again
	return 0
}

McastProtocol instproc handle-cache-miss { srcID group iface } {
	# return values: 
	#   0 : do not call classify on this packet again
	#   1 : changed iif for the corresponding mfc-entry, classify again
	return 0
}

McastProtocol instproc annotate args {
	$self instvar dynT_ node_ ns_
	set s "[$ns_ now] [$node_ id] $args" ;#nam wants uinique first arg???
	if [info exists dynT_] {
		foreach tr $dynT_ {
			$tr annotate $s
		}
	}
}

McastProtocol instproc join-group arg	{ 
	$self annotate $proc $arg 
}
McastProtocol instproc leave-group arg	{ 
	$self annotate $proc $arg
}

McastProtocol instproc trace { f src {op ""} } {
        $self instvar ns_ dynT_
	if {$op == "nam" && [info exists dynT_] > 0} {
		foreach tr $dynT_ {
			$tr namattach $f
		}
	} else {
		lappend dynT_ [$ns_ create-trace Generic $f $src $src $op]
	}
}
# This method is called when a change in routing occurs.
McastProtocol instproc notify { dummy } {
        $self instvar ns_ node_ PruneTimer_

	#build list of current sources
        foreach r [$node_ getReps "*" "*"] {
		set src_id [$r set srcID_]
		set sources($src_id) 1
	}
	set sourceIDs [array names sources]
	foreach src_id $sourceIDs {
		set src [$ns_ get-node-by-id $src_id]
		if {$src != $node_} {
			set upstream [$node_ rpf-nbr $src]
			if { $upstream != ""} {
				set inlink [$ns_ link $upstream $node_]
				set newiif [$node_ link2iif $inlink]
				set reps [$node_ getReps $src_id "*"]
				foreach r $reps {
					set oldiif [$node_ lookup-iface $src_id [$r set grp_]]
					if { $oldiif != $newiif } {
						$node_ change-iface $src_id [$r set grp_] $oldiif $newiif
					}
				}
			}
		}
		#next update outgoing interfaces
		set oiflist ""
		foreach nbr [$node_ neighbors] {
			set nbr_id [$nbr id]
			set nh [$nbr rpf-nbr $src] 
			if { $nh != $node_ } {
				# are we ($node_) the next hop from ($nbr) to 
				# the source ($src)
				continue
			}
			set oif [$node_ link2oif [$ns_ link $node_ $nbr]]
			# oif to such neighbor
			set oifs($oif) 1
		}
		set oiflist [array names oifs]

		set reps [$node_ getReps $src_id "*"]
		foreach r $reps {
			set grp [$r set grp_]
			set oldoifs [$r dump-oifs]
			set newoifs $oiflist
			foreach old $oldoifs {
				if [catch "$node_ oif2link $old" ] {
					# this must be a local agent, not an oif
					continue
				}
				set idx [lsearch $newoifs $old]
				if { $idx < 0} {
					$r disable $old
					if [info exists PruneTimer_($src_id:$grp:$old)] {
						delete $PruneTimer_($src_id:$grp:$old)
						unset PruneTimer_($src_id:$grp:$old)
					}
				} else {
					set newoifs [lreplace $newoifs $idx $idx]
				}
			}
			foreach new $newoifs {
				foreach r $reps {
					$r insert $new
				}
			}
		}
	}
}

McastProtocol instproc dump-routes {chan {grp ""} {src ""}} {
	$self instvar ns_ node_
	if { $grp == "" } {
		# dump all replicator entries
		array set reps [$node_ getReps-raw * *]
	} elseif { $src == "" } {
		# dump entries for group
		array set reps [$node_ getReps-raw * $grp]  ;# actually, more than *,g
	} else {
		# dump entries for src, group.
		array set reps [$node_ getReps-raw $src $grp]
	}
	puts $chan [concat "Node:\t${node_}([$node_ id])\tat t ="	\
			[format "%4.2f" [$ns_ now]]]
	puts $chan "\trepTag\tActive\t\tsrc\tgroup\tiifNode\t\tdest_nodes"
	foreach ent [lsort [array names reps]] {
		set sg [split $ent ":"]
		if { [$reps($ent) is-active] } {
			set active Y
		} else {
			set active N
		}
		# translate each oif to a link and then the neighbor node
		set dest ""
		foreach oif [$reps($ent) dump-oifs] {
			if ![catch { set nbr [[$node_ oif2link $oif] dst] } ] {
				set nbrid [$nbr id]
				if [$nbr is-lan?] {
					set nbrid ${nbrid}(L)
				}
				lappend dest $nbrid
			}
		}
		set s [lindex $sg 0]
		set g [lindex $sg 1]
		set iif [$node_ lookup-iface $s $g]

		set iif_node_id $iif
		catch {
			# catch: iif can be negative for senders
			set iif_node [[$node_ iif2link $iif] src]
			if [$iif_node is-lan?] {
				set iif_node_id [$iif_node id](L)
			} else {
				set iif_node_id [$iif_node id]
			}
		}

		puts $chan [format "\t%5s\t  %s\t\t%d\t0x%x\t%s\t\t%s"	\
				$reps($ent) $active $s $g $iif_node_id $dest]
	}
}


###################################################
Class mrtObject

#XXX well-known groups (WKG) with local multicast/broadcast
mrtObject set mask-wkgroups	0xfff0
mrtObject set wkgroups(Allocd)	[mrtObject set mask-wkgroups]

mrtObject proc registerWellKnownGroups name {
	set newGroup [mrtObject set wkgroups(Allocd)]
	mrtObject set wkgroups(Allocd) [expr $newGroup + 1]
	mrtObject set wkgroups($name)  $newGroup
}

mrtObject proc getWellKnownGroup name {
	assert "\"$name\" != \"Allocd\""
	mrtObject set wkgroups($name)
}

mrtObject registerWellKnownGroups ALL_ROUTERS
mrtObject registerWellKnownGroups ALL_PIM_ROUTERS

mrtObject proc expandaddr {} {
	# extend the space to 32 bits
	mrtObject set mask-wkgroups	0x7fffffff

	foreach {name group} [mrtObject array get wkgroups] {
		mrtObject set wkgroups($name) [expr $group | 0x7fffffff]
	}
}

mrtObject instproc init { node } {
        $self next
	$self set node_	     $node
}

mrtObject instproc addproto { proto { iiflist "" } } {
        $self instvar node_ protocols_
	# if iiflist is empty, protocol runs on all iifs
	if { $iiflist == "" } {
		set iiflist [$node_ get-all-iifs]
		lappend iiflist -1 ;#for local packets
	}
	foreach iif $iiflist {
		set protocols_($iif) $proto
	}
}

mrtObject instproc getType { protocolType } {
        $self instvar protocols_
        foreach iif [array names protocols_] {
                if { [$protocols_($iif) getType] == $protocolType } {
                        return $protocols_($iif)
                }
        }
        return ""
}

mrtObject instproc all-mprotos {op args} {
	$self instvar protocols_
	foreach iif [array names protocols_] {
		set p $protocols_($iif)
		if ![info exists protos($p)] {
			set protos($p) 1
			eval $p $op $args
		}
	}
}

mrtObject instproc start {}	{ $self all-mprotos start	}
mrtObject instproc stop {}	{ $self all-mprotos stop	}
mrtObject instproc notify dummy { $self all-mprotos notify $dummy }
mrtObject instproc dump-routes args {
	$self all-mprotos dump-routes $args
}

# similar to membership indication by igmp.. 
mrtObject instproc join-group { grp src } {
	eval $self all-mprotos join-group $grp $src
}

mrtObject instproc leave-group { grp src } {
	eval $self all-mprotos leave-group $grp $src
}

mrtObject instproc upcall { code source group iface } {
  # check if the group is local multicast to well-known group
	set wkgroup [expr [$class set mask-wkgroups]]
	if { [expr ( $group & $wkgroup ) == $wkgroup] } {
                $self instvar node_
		$node_ add-mfc $source $group -1 {}
		return 1
        } else {
		$self instvar protocols_
		$protocols_($iface) upcall $code $source $group $iface
	}
}

mrtObject instproc drop { replicator src dst {iface -1} } {
	$self instvar protocols_
	$protocols_($iface) drop $replicator $src $dst $iface
}
