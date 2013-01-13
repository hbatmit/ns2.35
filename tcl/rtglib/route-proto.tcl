#
#  Copyright (c) 1997 by the University of Southern California
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

# $Header: /cvsroot/nsnam/ns-2/tcl/rtglib/route-proto.tcl,v 1.31 2005/09/16 03:05:46 tomh Exp $

#
# Author: <kannan@isi.edu> (this email address has deprecated.)
#
#
# This file only contains the methods for dynamic routing.
# Check ../lib/ns-route.tcl for the Simulator (static) routing support
#

set rtglibRNG [new RNG]
$rtglibRNG seed 1

Class rtObject

rtObject set unreach_ -1
rtObject set maxpref_   255

# This may not be called by all routing agents. For instance, DV calls 
# this one but static does not. As a result, static routing does not have
# rtObject on any node.
rtObject proc init-all args {
    foreach node $args {
	if { [$node rtObject?] == "" } {
	    set rtobj($node) [new rtObject $node]
	}
    }
    foreach node $args {	;# XXX
        $rtobj($node) compute-routes
    }
}

rtObject instproc init node {
    $self next
    $self instvar ns_ nullAgent_
    $self instvar nextHop_ rtpref_ metric_ node_ rtVia_ rtProtos_

    set ns_ [Simulator instance]
    set nullAgent_ [$ns_ set nullAgent_]

    $node init-routing $self
    set node_ $node
    foreach dest [$ns_ all-nodes-list] {
	set nextHop_($dest) ""
	if {$node == $dest} {
	    set rtpref_($dest) 0
	    set metric_($dest) 0
	    set rtVia_($dest) "Agent/rtProto/Local" ;# make dump happy
	} else {
	    set rtpref_($dest) [$class set maxpref_]
	    set metric_($dest) [$class set unreach_]
	    set rtVia_($dest)    ""
	    $node add-route [$dest id] $nullAgent_
	}
    }
    $self add-proto Direct $node
    $rtProtos_(Direct) compute-routes
}

rtObject instproc add-proto {proto node} {
    $self instvar ns_ rtProtos_
    set rtProtos_($proto) [new Agent/rtProto/$proto $node]
    $ns_ attach-agent $node $rtProtos_($proto)
    set rtProtos_($proto)
}

rtObject instproc lookup dest {
    $self instvar nextHop_ node_
    if {![info exists nextHop_($dest)] || $nextHop_($dest) == ""} {
	return -1
    } else {
	return [[$nextHop_($dest) set toNode_] id]
    }
}

rtObject instproc compute-routes {} {
    # choose the best route to each destination from all protocols
    $self instvar ns_ node_ rtProtos_ nullAgent_
    $self instvar nextHop_ rtpref_ metric_ rtVia_
    set protos ""
    set changes 0
    foreach p [array names rtProtos_] {
	if [$rtProtos_($p) set rtsChanged_] {
	    incr changes
	    $rtProtos_($p) set rtsChanged_ 0
	}
	lappend protos $rtProtos_($p)
    }
    if !$changes return

    set changes 0
    foreach dst [$ns_ all-nodes-list] {
	if {$dst == $node_} continue
	set nh ""
	set pf [$class set maxpref_]
	set mt [$class set unreach_]
	set rv ""
	foreach p $protos {
	    set pnh [$p set nextHop_($dst)]
	    if { $pnh == "" } continue

	    set ppf [$p set rtpref_($dst)]
	    set pmt [$p set metric_($dst)]
	    if {$ppf < $pf || ($ppf == $pf && $pmt < $mt) || $mt < 0} {
		set nh  $pnh
		set pf  $ppf
		set mt  $pmt
		set rv  $p
	    }
	}
	if { $nh == "" } {
	    # no route...  delete any existing routes
	    if { $nextHop_($dst) != "" } {
		$node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		set nextHop_($dst) $nh
		set rtpref_($dst)  $pf
		set metric_($dst)  $mt
		set rtVia_($dst)   $rv
		incr changes
	    }
	} else {
	    if { $rv == $rtVia_($dst) } {
		# Current protocol still has best route.
		# See if changed
		if { $nh != $nextHop_($dst) } {
		    $node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		    set nextHop_($dst) $nh
		    $node_ add-routes [$dst id] $nextHop_($dst)
		    incr changes
		}
		if { $mt != $metric_($dst) } {
		    set metric_($dst) $mt
		    incr changes
		}
		if { $pf != $rtpref_($dst) } {
		    set rtpref_($dst) $pf
		}
	    } else {
		if { $rtVia_($dst) != "" } {
		    set nextHop_($dst) [$rtVia_($dst) set nextHop_($dst)]
		    set rtpref_($dst)  [$rtVia_($dst) set rtpref_($dst)]
		    set metric_($dst)  [$rtVia_($dst) set metric_($dst)]
		}
		if {$rtpref_($dst) != $pf || $metric_($dst) != $mt} {
		    # Then new prefs must be better, or
		    # new prefs are equal, and new metrics are lower
		    $node_ delete-routes [$dst id] $nextHop_($dst) $nullAgent_
		    set nextHop_($dst) $nh
		    set rtpref_($dst)  $pf
		    set metric_($dst)  $mt
		    set rtVia_($dst)   $rv
		    $node_ add-routes [$dst id] $nextHop_($dst)
		    incr changes
		}
	    }
	}
    }
    foreach proto [array names rtProtos_] {
	$rtProtos_($proto) send-updates $changes
    }
    #
    # XXX
    # detailed multicast routing hooks must come here.
    # My idea for the hook will be something like:
    # set mrtObject [$node_ mrtObject?]
    # if {$mrtObject != ""} {
    #    $mrtObject recompute-mroutes $changes
    # }
    # $changes == 0	if only interfaces changed state.  Look at how
    #			Agent/rtProto/DV handles ifsUp_
    # $changes > 0	if new unicast routes were installed.
    #
    $self flag-multicast $changes
}

rtObject instproc flag-multicast changes {
    $self instvar node_
    $node_ notify-mcast $changes
}

rtObject instproc intf-changed {} {
    $self instvar ns_ node_ rtProtos_ rtVia_ nextHop_ rtpref_ metric_
    foreach p [array names rtProtos_] {
	$rtProtos_($p) intf-changed
	$rtProtos_($p) compute-routes
    }
    $self compute-routes
}

rtObject instproc dump-routes chan {
    $self instvar ns_ node_ nextHop_ rtpref_ metric_ rtVia_

    if {$ns_ != ""} {
	set time [$ns_ now]
    } else {
	set time 0.0
    }
    puts $chan [concat "Node:\t${node_}([$node_ id])\tat t ="		\
	    [format "%4.2f" $time]]
    puts $chan "  Dest\t\t nextHop\tPref\tMetric\tProto"
    #foreach dest [lsort -command SplitObjectCompare [$ns_ all-nodes-list]] {
    foreach dest [$ns_ all-nodes-list] {
	if {[llength $nextHop_($dest)] > 1} {
	    set p [split [$rtVia_($dest) info class] /]
	    set proto [lindex $p [expr [llength $p] - 1]]
	    foreach rt $nextHop_($dest) {
		puts $chan [format "%-5s(%d)\t%-5s(%d)\t%3d\t%4d\t %s"	 \
			$dest [$dest id] $rt [[$rt set toNode_] id]	 \
			$rtpref_($dest) $metric_($dest) $proto]
	    }
	} elseif {$nextHop_($dest) != ""} {
	    set p [split [$rtVia_($dest) info class] /]
	    set proto [lindex $p [expr [llength $p] - 1]]
	    puts $chan [format "%-5s(%d)\t%-5s(%d)\t%3d\t%4d\t %s"	 \
		    $dest [$dest id]					 \
		    $nextHop_($dest) [[$nextHop_($dest) set toNode_] id] \
		    $rtpref_($dest) $metric_($dest) $proto]
	} elseif {$dest == $node_} {
	    puts $chan [format "%-5s(%d)\t%-5s(%d)\t%03d\t%4d\t %s"	\
		    $dest [$dest id] $dest [$dest id] 0 0 "Local"]
	} else {
	    puts $chan [format "%-5s(%d)\t%-5s(%s)\t%03d\t%4d\t %s"	\
		    $dest [$dest id] "" "-" 255 32 "Unknown"]
	}
    }
}

rtObject instproc rtProto? proto {
    $self instvar rtProtos_
    if [info exists rtProtos_($proto)] {
	return $rtProtos_($proto)
    } else {
	return ""
    }
}

rtObject instproc nextHop? dest {
    $self instvar nextHop_
    $self set nextHop_($dest)
}

rtObject instproc rtpref? dest {
    $self instvar rtpref_
    $self set rtpref_($dest)
}

rtObject instproc metric? dest {
    $self instvar metric_
    $self set metric_($dest)
}

#
Class rtPeer

rtPeer instproc init {addr port cls} {
    $self next
    $self instvar addr_ port_ metric_ rtpref_
    set addr_ $addr
    set port_ $port
    foreach dest [[Simulator instance] all-nodes-list] {
	set metric_($dest) [$cls set INFINITY]
	set rtpref_($dest) [$cls set preference_]
    }
}

rtPeer instproc addr? {} {
    $self instvar addr_
    return $addr_
}

rtPeer instproc port? {} {
    $self instvar port_
    return $port_
}

rtPeer instproc metric {dest val} {
    $self instvar metric_
    set metric_($dest) $val
}

rtPeer instproc metric? dest {
    $self instvar metric_
    return $metric_($dest)
}

rtPeer instproc preference {dest val} {
    $self instvar rtpref_
    set rtpref_($dest) $val
}

rtPeer instproc preference? dest {
    $self instvar rtpref_
    return $rtpref_($dest)
}

#
#Class Agent/rtProto -superclass Agent

Agent/rtProto proc pre-init-all args {
    # By default, do nothing when a person does $ns rtproto foo.
}

Agent/rtProto proc init-all args {
    error "No initialization defined"
}

Agent/rtProto instproc init node {
    $self next
    
    $self instvar ns_ node_ rtObject_ preference_ ifs_ ifstat_
    set ns_ [Simulator instance]

    catch "set preference_ [[$self info class] set preference_]" ret
    if { $ret == "" } {
	set preference_ [$class set preference_]
    }
    foreach nbr [$node set neighbor_] {
	set link [$ns_ link $node $nbr]
	set ifs_($nbr) $link
	set ifstat_($nbr) [$link up?]
    }
    set rtObject_ [$node rtObject?]
}

Agent/rtProto instproc compute-routes {} {
    error "No route computation defined"
}

Agent/rtProto instproc intf-changed {} {
    #NOTHING
}

Agent/rtProto instproc send-updates args {
    #NOTHING
}

Agent/rtProto proc compute-all {} {
    #NOTHING
}

#
# Static routing, the default
#
Class Agent/rtProto/Static -superclass Agent/rtProto

Agent/rtProto/Static proc init-all args {
    # The Simulator knows the entire topology.
    # Hence, the current compute-routes method in the Simulator class is
    # well suited.  We use it as is.
    [Simulator instance] compute-routes
}

#
# Session based unicast routing
#
Class Agent/rtProto/Session -superclass Agent/rtProto

Agent/rtProto/Session proc init-all args {
    [Simulator instance] compute-routes
}

Agent/rtProto/Session proc compute-all {} {
    [Simulator instance] compute-routes
}

#
#########################################################################
#
# Code below this line is experimental, and should be considered work
# in progress.  None of this code is used in production test-suites, or
# in the release yet, and hence should not be a problem to anyone.
#
Class Agent/rtProto/Direct -superclass Agent/rtProto
Agent/rtProto/Direct instproc init node {
    $self next $node
    $self instvar ns_ rtpref_ nextHop_ metric_ ifs_

    foreach node [$ns_ all-nodes-list] {
	set rtpref_($node) 255
	set nextHop_($node) ""
	set metric_($node) -1
    }
    foreach node [array names ifs_] {
	set rtpref_($node) [$class set preference_]
    }
}

Agent/rtProto/Direct instproc compute-routes {} {
    $self instvar ifs_ ifstat_ nextHop_ metric_ rtsChanged_
    set rtsChanged_ 0
    foreach nbr [array names ifs_] {
	if {$nextHop_($nbr) == "" && [$ifs_($nbr) up?] == "up"} {
	    set ifstat_($nbr) 1
	    set nextHop_($nbr) $ifs_($nbr)
	    set metric_($nbr) [$ifs_($nbr) cost?]
	    incr rtsChanged_
	} elseif {$nextHop_($nbr) != "" && [$ifs_($nbr) up?] != "up"} {
	    set ifstat_($nbr) 0
	    set nextHop_($nbr) ""
	    set metric_($nbr) -1
	    incr rtsChanged_
	}
    }
}

#
# Distance Vector Route Computation
#
# Class Agent/rtProto/DV -superclass Agent/rtProto
Agent/rtProto/DV set UNREACHABLE	[rtObject set unreach_]
Agent/rtProto/DV set mid_		  0

Agent/rtProto/DV proc init-all args {
    if { [llength $args] == 0 } {
	set nodeslist [[Simulator instance] all-nodes-list]
    } else {
	eval "set nodeslist $args"
    }
    Agent set-maxttl Agent/rtProto/DV INFINITY
    eval rtObject init-all $nodeslist
    foreach node $nodeslist {
	set proto($node) [[$node rtObject?] add-proto DV $node]
    }
    foreach node $nodeslist {
	foreach nbr [$node neighbors] {
	    set rtobj [$nbr rtObject?]
	    if { $rtobj != "" } {
		set rtproto [$rtobj rtProto? DV]
		if { $rtproto != "" } {
		    $proto($node) add-peer $nbr [$rtproto set agent_addr_] [$rtproto set agent_port_]
		}
	    }
	}
    }
}

Agent/rtProto/DV instproc init node {
    global rtglibRNG

    $self next $node
    $self instvar ns_ rtObject_ ifsUp_
    $self instvar preference_ rtpref_ nextHop_ nextHopPeer_ metric_ multiPath_

    set UNREACHABLE [$class set UNREACHABLE]
    foreach dest [$ns_ all-nodes-list] {
	set rtpref_($dest) $preference_
	set nextHop_($dest) ""
	set nextHopPeer_($dest) ""
	set metric_($dest)  $UNREACHABLE
    }
    set ifsUp_ ""
    set multiPath_ [[$rtObject_ set node_] set multiPath_]
    set updateTime [$rtglibRNG uniform 0.0 0.5]
    $ns_ at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc add-peer {nbr agentAddr agentPort} {
    $self instvar peers_
    $self set peers_($nbr) [new rtPeer $agentAddr $agentPort $class]
}

Agent/rtProto/DV instproc send-periodic-update {} {
    global rtglibRNG

    $self instvar ns_
    $self send-updates 1	;# Anything but 0
    set updateTime [expr [$ns_ now] + \
	    ([$class set advertInterval] * [$rtglibRNG uniform 0.9 1.1])]
    $ns_ at $updateTime "$self send-periodic-update"
}

Agent/rtProto/DV instproc compute-routes {} {
    $self instvar ns_ ifs_ rtpref_ metric_ nextHop_ nextHopPeer_
    $self instvar peers_ rtsChanged_ multiPath_

    set INFINITY [$class set INFINITY]
    set MAXPREF  [rtObject set maxpref_]
    set UNREACH	 [rtObject set unreach_]
    set rtsChanged_ 0
    foreach dst [$ns_ all-nodes-list] {
	set p [lindex $nextHopPeer_($dst) 0]
	if {$p != ""} {
	    set metric_($dst) [$p metric? $dst]
	    set rtpref_($dst) [$p preference? $dst]
	}

	set pf $MAXPREF
	set mt $INFINITY
	set nh(0) 0
	foreach nbr [lsort -dictionary [array names peers_]] {
	    set pmt [$peers_($nbr) metric? $dst]
	    set ppf [$peers_($nbr) preference? $dst]

	    # if peer metric not valid	continue
	    # if peer pref higher		continue
	    # if peer pref lower		set to latest values
	    # else peer pref equal
	    #	if peer metric higher	continue
	    #	if peer metric lower	set to latest values
	    #	else peer metrics equal	append latest values

	    if { $pmt < 0 || $pmt >= $INFINITY || $ppf > $pf || $pmt > $mt } \
		    continue
	    if { $ppf < $pf || $pmt < $mt } {
		set pf $ppf
		set mt $pmt
		unset nh	;# because we must compute *new* next hops
	    }
	    set nh($ifs_($nbr)) $peers_($nbr)
	}
	catch "unset nh(0)"
	if { $pf == $MAXPREF && $mt == $INFINITY } continue
	if { $pf > $rtpref_($dst) ||				\
		($metric_($dst) >= 0 && $mt > $metric_($dst)) }	\
		continue
	if {$mt >= $INFINITY} {
	    set mt $UNREACH
	}

	incr rtsChanged_
	if { $pf < $rtpref_($dst) || $mt < $metric_($dst) } {
	    set rtpref_($dst) $pf
	    set metric_($dst) $mt
	    set nextHop_($dst) ""
	    set nextHopPeer_($dst) ""
	    foreach n [array names nh] {
		lappend nextHop_($dst) $n
		lappend nextHopPeer_($dst) $nh($n)
		if !$multiPath_ break;
	    }
	    continue
	}
	
	set rtpref_($dst) $pf
	set metric_($dst) $mt
	set newNextHop ""
	set newNextHopPeer ""
	foreach rt $nextHop_($dst) {
	    if [info exists nh($rt)] {
		lappend newNextHop $rt
		lappend newNextHopPeer $nh($rt)
		unset nh($rt)
	    }
	}
	set nextHop_($dst) $newNextHop
	set nextHopPeer_($dst) $newNextHopPeer
	if { $multiPath_ || $nextHop_($dst) == "" } {
	    foreach rt [array names nh] {
		lappend nextHop_($dst) $rt
		lappend nextHopPeer_($dst) $nh($rt)
		if !$multiPath_ break
	    }
	}
    }
    set rtsChanged_
}

Agent/rtProto/DV instproc intf-changed {} {
    $self instvar ns_ peers_ ifs_ ifstat_ ifsUp_ nextHop_ nextHopPeer_ metric_
    set INFINITY [$class set INFINITY]
    set ifsUp_ ""
    foreach nbr [lsort -dictionary [array names peers_]] {
	set state [$ifs_($nbr) up?]
	if {$state != $ifstat_($nbr)} {
	    set ifstat_($nbr) $state
	    if {$state != "up"} {
		if ![info exists all-nodes] {
		    set all-nodes [$ns_ all-nodes-list]
		}
		foreach dest ${all-nodes} {
		    $peers_($nbr) metric $dest $INFINITY
		}
	    } else {
		lappend ifsUp_ $nbr
	    }
	}
    }
}

Agent/rtProto/DV proc get-next-mid {} {
    set ret [Agent/rtProto/DV set mid_]
    Agent/rtProto/DV set mid_ [expr $ret + 1]
    set ret
}

Agent/rtProto/DV proc retrieve-msg id {
    set ret [Agent/rtProto/DV set msg_($id)]
    Agent/rtProto/DV unset msg_($id)
    set ret
}

Agent/rtProto/DV instproc send-updates changes {
    $self instvar peers_ ifs_ ifsUp_

    if $changes {
	set to-send-to [lsort -dictionary [array names peers_]]
    } else {
	set to-send-to $ifsUp_
    }
    set ifsUp_ ""
    foreach nbr ${to-send-to} {
	if { [$ifs_($nbr) up?] == "up" } {
	    $self send-to-peer $nbr
	}
    }
}

Agent/rtProto/DV instproc send-to-peer nbr {
    $self instvar ns_ rtObject_ ifs_ peers_
    set INFINITY [$class set INFINITY]
    foreach dest [$ns_ all-nodes-list] {
	set metric [$rtObject_ metric? $dest]
	if {$metric < 0} {
	    set update($dest) $INFINITY
	} else {
	    set update($dest) [$rtObject_ metric? $dest]
	    foreach nh [$rtObject_ nextHop? $dest] {
		if {$nh == $ifs_($nbr)} {
		    set update($dest) $INFINITY
		}
	    }
	}
    }

    ### modifed by Liang Guo, 11/11/99, what if there's no peer on that end?
    ### needed when only part of the network nodes are using DV routing
    if { $peers_($nbr) == "" } {
        return
    }
    ##################### End ##########

    set id [$class get-next-mid]
    $class set msg_($id) [array get update]

    # XXX Note the singularity below...
    $self send-update [$peers_($nbr) addr?] [$peers_($nbr) port?] $id [array size update]
}

Agent/rtProto/DV instproc recv-update {peerAddr id} {
    $self instvar peers_ ifs_ nextHopPeer_ metric_
    $self instvar rtsChanged_ rtObject_

    set INFINITY [$class set INFINITY]
    set UNREACHABLE  [$class set UNREACHABLE]
    set msg [$class retrieve-msg $id]
    array set metrics $msg
    foreach nbr [lsort -dictionary [array names peers_]] {
	if {[$peers_($nbr) addr?] == $peerAddr} {
	    set peer $peers_($nbr)
	    if { [array size metrics] > [Node set nn_] } {
		error "$class::$proc update $peerAddr:$msg:$count is larger than the simulation topology"
	    }
	    set metricsChanged 0
	    foreach dest [array names metrics] {
                set metric [expr $metrics($dest) + [$ifs_($nbr) cost?]]
		if {$metric > $INFINITY} {
		    set metric $INFINITY
		}
		if {$metric != [$peer metric? $dest]} {
		    $peer metric $dest $metric
		    incr metricsChanged
		}
	    }
	    if $metricsChanged {
		$self compute-routes
		incr rtsChanged_ $metricsChanged
		$rtObject_ compute-routes
	    } else {
		# dynamicDM multicast hack.
		# If we get a message from a neighbour, then something
		# at that neighbour has changed.  While this may not
		# cause any unicast changes on our end, dynamicDM
		# looks at neighbour's routing tables to compute
		# parent-child relationships, and has to do them
		# again.
		#
		$rtObject_ flag-multicast -1
	    }
	    return
	}
    }
    error "$class::$proc update $peerAddr:$msg:$count from unknown peer"
}

Agent/rtProto/DV proc compute-all {} {
    # Because proc methods are not inherited from the parent class.
}

#
# Manual routing
#
Class Agent/rtProto/Manual -superclass Agent/rtProto

Agent/rtProto/Manual proc pre-init-all args {
    Node enable-module Manual
}

Agent/rtProto/Manual proc init-all args {
    # The user will do all routing.
}

### Local Variables:
### mode: tcl
### tcl-indent-level: 4
### tcl-default-application: ns
### End:
