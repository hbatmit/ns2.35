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
# 
# ns trace support for nam
#
# Author: Haobo Yu (haoboy@isi.edu)
#
# $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-namsupp.tcl,v 1.43 2006/02/22 13:23:15 mahrenho Exp $
#

#
# Support for node tracing
#

# This will only work during initialization. Not possible to change shape 
# dynamically
Node instproc shape { shape } {
	$self instvar attr_ 
	set attr_(SHAPE) $shape
}

# Returns the current shape of the node
Node instproc get-shape {} {
	$self instvar attr_
	if [info exists attr_(SHAPE)] {
		return $attr_(SHAPE)
	} else {
		return ""
	}
}

Node instproc color { color } {
	$self instvar attr_ id_

	set ns [Simulator instance]

	if [$ns is-started] {
		# color must be initialized

		$ns puts-nam-config \
		[eval list "n -t [format "%.15g" [$ns now]] -s $id_ -S COLOR -c $color -o $attr_(COLOR) -i $color -I $attr_(LCOLOR)"]
		set attr_(COLOR) $color
	        set attr_(LCOLOR) $color
	} else {
		set attr_(COLOR) $color
	        set attr_(LCOLOR) $color
	}
}

Node instproc label { str} {
	$self instvar attr_ id_

	set ns [Simulator instance]

	if [info exists attr_(DLABEL)] {
		$ns puts-nam-config "n -t [$ns now] -s $id_ -S DLABEL -l \"$str\" -L $attr_(DLABEL)"
	} else {
		$ns puts-nam-config "n -t [$ns now] -s $id_ -S DLABEL -l \"$str\" -L \"\""
	}
	set attr_(DLABEL) \"$str\"
}

Node instproc label-color { str} {
        $self instvar attr_ id_

        set ns [Simulator instance]

        if [info exists attr_(DCOLOR)] {
                $ns puts-nam-config "n -t [$ns now] -s $id_ -S DCOLOR -e \"$str\" -E $attr_(DCOLOR)"
        } else {
                $ns puts-nam-config "n -t [$ns now] -s $id_ -S DCOLOR -e \"$str\" -E \"\""
        }
        set attr_(DCOLOR) \"$str\"
}

Node instproc label-at { str } {
        $self instvar attr_ id_

        set ns [Simulator instance]

        if [info exists attr_(DIRECTION)] {
                $ns puts-nam-config "n -t [$ns now] -s $id_ -S DIRECTION -p \"$str\" -P $attr_(DIRECTION)"
        } else {
                $ns puts-nam-config "n -t [$ns now] -s $id_ -S DIRECTION -p \"$str\" -P \"\""
        }
        set attr_(DIRECTION) \"$str\"
}

Node instproc dump-namconfig {} {
	$self instvar attr_ id_ address_ X_ Y_ Z_
	set ns [Simulator instance]

	if ![info exists attr_(SHAPE)] {
		set attr_(SHAPE) "circle"
	} 
	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	        set attr_(LCOLOR) "black"
	}
        if ![info exists attr_(DCOLOR)] {
                set attr_(DCOLOR) "black"
        }
	if { [info exists X_] && [info exists Y_] } {
		if [info exists Z_] {
			$ns puts-nam-config \
			[eval list "n -t * -a $address_ -s $id_ -S UP -v $attr_(SHAPE) -c $attr_(COLOR) -i $attr_(LCOLOR) -x $X_ -y $Y_ -Z $Z_"]
		} else {
			$ns puts-nam-config \
			[eval list "n -t * -a $address_ -s $id_ -S UP -v $attr_(SHAPE) -c $attr_(COLOR) -i $attr_(LCOLOR) -x $X_ -y $Y_"]
		}
	} else {
	$ns puts-nam-config \
		[eval list "n -t * -a $address_ -s $id_ -S UP -v $attr_(SHAPE) -c $attr_(COLOR) -i $attr_(LCOLOR)"]
	}
}

Node instproc change-color { color } {
	puts "Warning: Node::change-color is obsolete. Use Node::color instead"
	$self color $color
}

Node instproc get-attribute { name } {
	$self instvar attr_
	if [info exists attr_($name)] {
		return $attr_($name)
	} else {
		return ""
	}
}

Node instproc get-color {} {
	puts "Warning: Node::get-color is obsolete. Please use Node::get-attribute"
	return [$self get-attribute "COLOR"]
}

Node instproc add-mark { name color {shape "circle"} } {
	$self instvar id_ markColor_ shape_
	set ns [Simulator instance]

	$ns puts-nam-config "m -t [$ns now] -s $id_ -n $name -c $color -h $shape"
	set markColor_($name) $color
	set shape_($name) $shape
}

Node instproc delete-mark { name } {
	$self instvar id_ markColor_ shape_

	# Ignore if the mark $name doesn't exist
	if ![info exists markColor_($name)] {
		return
	}

	set ns [Simulator instance]
	$ns puts-nam-config \
		"m -t [$ns now] -s $id_ -n $name -c $markColor_($name) -h $shape_($name) -X"
}

#
# Support for link tracing
# XXX only SimpleLink (and its children) can dump nam config, because Link
# doesn't have bandwidth and delay.
#
SimpleLink instproc dump-namconfig {} {
	# make a duplex link in nam
	$self instvar link_ attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}

	set ns [Simulator instance]
	set bw [$link_ set bandwidth_]
	set delay [$link_ set delay_]

	if [info exists attr_(ORIENTATION)] {
		$ns puts-nam-config \
			"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -c $attr_(COLOR) -o $attr_(ORIENTATION)"
	} else {
		$ns puts-nam-config \
			"l -t * -s [$fromNode_ id] -d [$toNode_ id] -S UP -r $bw -D $delay -c $attr_(COLOR)"
	}
}

Link instproc dump-nam-queueconfig {} {
	$self instvar attr_ fromNode_ toNode_

	if ![info exists attr_(COLOR)] {
		set attr_(COLOR) "black"
	}

	set ns [Simulator instance]
	if [info exists attr_(QUEUE_POS)] {
		$ns puts-nam-config "q -t * -s [$fromNode_ id] -d [$toNode_ id] -a $attr_(QUEUE_POS)"
	} else {
		set attr_(QUEUE_POS) ""
	}
}

#
# XXX
# This function should be called ONLY ONCE during initialization. 
# The order in which links are created in nam is determined by the calling 
# order of this function.
#
Link instproc orient { ori } {
	$self instvar attr_
	set attr_(ORIENTATION) $ori
	[Simulator instance] register-nam-linkconfig $self
}

Link instproc get-attribute { name } {
	$self instvar attr_
	if [info exists attr_($name)] {
		return $attr_($name)
	} else {
		return ""
	}
}

Link instproc queuePos { pos } {
	$self instvar attr_
	set attr_(QUEUE_POS) $pos
}

Link instproc color { color } {
	$self instvar attr_ fromNode_ toNode_ trace_

	set ns [Simulator instance]
	if [$ns is-started] {
		$ns puts-nam-config \
			[eval list "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S COLOR -c $color -o $attr_(COLOR)"]
		set attr_(COLOR) $color
	} else {
		set attr_(COLOR) $color
	}
}

# a link doesn't have its own trace file, write it to global trace file
Link instproc change-color { color } {
	puts "Warning: Link::change-color is obsolete. Please use Link::color."
	$self color $color
}

Link instproc get-color {} {
	puts "Warning: Node::get-color is obsolete. Please use Node::get-attribute"
	return [$self get-attribute "COLOR"]
}

Link instproc label { label } {
        $self instvar attr_ fromNode_ toNode_ trace_
        set ns [Simulator instance]
        if [info exists attr_(DLABEL)] {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DLABEL -l \"$label\" -L $attr_(DLABEL)"
        } else {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DLABEL -l \"$label\" -L \"\""
        }
        set attr_(DLABEL) \"$label\"
    }

Link instproc label-color { str } {
        $self instvar attr_ fromNode_ toNode_ trace_
        set ns [Simulator instance]
        if [info exists attr_(DCOLOR)] {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DCOLOR -e \"$str\" -E $attr_(DCOLOR)"
        } else {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DCOLOR -e \"$str\" -E \"\""
        }
        set attr_(DCOLOR) \"$str\"
    }

Link instproc label-at { str } {
        $self instvar attr_ fromNode_ toNode_ trace_
        set ns [Simulator instance]
        if [info exists attr_(DIRECTION)] {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DIRECTION -p \"$str\" -P $attr_(DIRECTION)"
        } else {
            $ns puts-nam-config \
            "l -t [$ns now] -s [$fromNode_ id] -d [$toNode_ id] -S DIRECTION -p \"$str\" -P \"\""
        }
        set attr_(DIRECTION) \"$str\"
    }

#
# Support for nam snapshot
#

Simulator instproc snapshot { } {
    set ns [Simulator instance]
    $ns puts-nam-config \
            "v -t [$self now] take_snapshot"
}

Simulator instproc rewind-nam { } {
	set ns [Simulator instance]
	$ns puts-nam-config \
			"v  -t [$self now] playing_backward"
}

Simulator instproc re-rewind-nam { } {
	set ns [Simulator instance]
	$ns puts-nam-config \
				"v  -t [$self now] playing_forward"
}

Simulator instproc terminate-nam { } {
	set ns [Simulator instance]
	$ns puts-nam-config \
				"v  -t [$self now] terminating_nam"
}

#
# Support for agent tracing
#

# This function records agents being traced, so they will be written into nam
# trace when the simulator starts
Simulator instproc add-agent-trace { agent name {f ""} } {
	$self instvar tracedAgents_
	set tracedAgents_($name) $agent

	set trace [$self get-nam-traceall]
	if {$f != ""} {
		$agent attach-trace $f
	} elseif {$trace != ""} {
		$agent attach-trace $trace
	}
}

Simulator instproc delete-agent-trace { agent } {
	$agent delete-agent-trace
}

Simulator instproc monitor-agent-trace { agent } {
	$self instvar monitoredAgents_
	lappend monitoredAgents_ $agent
}

#
# Agent trace is added when attaching to a traced node
# we need to keep a file handle in tcl so that var tracing can also be 
# done in tcl by manual inserting update-var-trace{}
#
Agent instproc attach-trace { file } {
	$self instvar namTrace_
	set namTrace_ $file 
	# add all traced var messages
	$self attach $file 
}

#
# nam initialization
#
Simulator instproc dump-namagents {} {
	$self instvar tracedAgents_ monitoredAgents_
	
	if {![$self is-started]} {
		return
	}
	if [info exists tracedAgents_] {
		foreach id [array names tracedAgents_] {
			$tracedAgents_($id) add-agent-trace $id
			$tracedAgents_($id) cmd dump-namtracedvars
		}
		unset tracedAgents_
	}
	if [info exists monitoredAgents_] {
		foreach a $monitoredAgents_ {
			$a show-monitor
		}
		unset monitoredAgents_
	}
}

Simulator instproc dump-namversion { v } {
	$self puts-nam-config "V -t * -v $v -a 0"
}

Simulator instproc dump-namwireless {} {
	$self instvar namNeedsW_ namWx_ namWy_

	# see if we need to write a W event
	if ![info exists namNeedsW_] { set namNeedsW_ 0 }
	if {[info exists namWx_] && [info exists namWy_]}  {
		set maxX $namWx_
		set maxY $namWy_
	} else {
		set maxX 10
		set maxY 10

		# get max X and Y coords of nodes
		# if any nodes have coordinates set, then flag the need for
		# a W event and adjust maxX/maxY as needed
		foreach node [Node info instances] {
			if {[lsearch -exact [$node info vars] X_] != -1} {
				set namNeedsW_ 1
				set curX [$node set X_]
				if {$curX > $maxX} {set maxX $curX}
			}
			if {[lsearch -exact [$node info vars] Y_] != -1} {
				set namNeedsW_ 1
				set curY [$node set Y_]
				if {$curY > $maxY} {set maxY $curY}
			}
		}
	}

	if $namNeedsW_ {
		$self puts-nam-config "W -t * -x $maxX -y $maxY"
	}
}

Simulator instproc dump-namcolors {} {
	$self instvar color_
	if ![$self is-started] {
		return 
	}
	foreach id [array names color_] {
		$self puts-nam-config "c -t * -i $id -n $color_($id)"
	}
}

Simulator instproc dump-namlans {} {
	if ![$self is-started] {
		return
	}
	$self instvar Node_
	foreach nn [array names Node_] {
		if [$Node_($nn) is-lan?] {
			$Node_($nn) dump-namconfig
		}
	}
}

Simulator instproc dump-namlinks {} {
	$self instvar linkConfigList_
	if ![$self is-started] {
		return
	}
	if [info exists linkConfigList_] {
		foreach lnk $linkConfigList_ {
			$lnk dump-namconfig
		}
		unset linkConfigList_
	}
}

Simulator instproc dump-namnodes {} {
	$self instvar Node_
	if ![$self is-started] {
		return
	}
	foreach nn [array names Node_] {
		if ![$Node_($nn) is-lan?] {
			$Node_($nn) dump-namconfig
		}
	}
}

Simulator instproc dump-namqueues {} {
	$self instvar link_
	if ![$self is-started] {
		return
	}
	foreach qn [array names link_] {
		$link_($qn) dump-nam-queueconfig
	}
}

# Write hierarchical masks/shifts into trace file
Simulator instproc dump-namaddress {} {
	# First write number of hierarchies
	$self puts-nam-config \
	    "A -t * -n [AddrParams hlevel] -p 0 -o [AddrParams set \
ALL_BITS_SET] -c [AddrParams McastShift] -a [AddrParams McastMask]"
	
	for {set i 1} {$i <= [AddrParams hlevel]} {incr i} {
		$self puts-nam-config "A -t * -h $i -m [AddrParams \
NodeMask $i] -s [AddrParams NodeShift $i]"
	}
}

Simulator instproc init-nam {} {
	$self instvar annotationSeq_ 

	set annotationSeq_ 0

	# Setting nam trace file version first
	$self dump-namversion 1.0a5
	
	# write W event if needed
	$self dump-namwireless

	# Addressing scheme
	$self dump-namaddress
	
	# Color configuration for nam
	$self dump-namcolors
	
	# Node configuration for nam
	$self dump-namnodes
	
	# Lan and Link configurations for nam
	$self dump-namlinks 
	$self dump-namlans
	
	# nam queue configurations
	$self dump-namqueues
	
	# Traced agents for nam
	$self dump-namagents

}

#
# Other animation control support
# 
Simulator instproc trace-annotate { str } {
	$self instvar annotationSeq_
	$self puts-ns-traceall [format \
		"v %s %s {set sim_annotation {%s}}" [$self now] eval $str]
	incr annotationSeq_
	$self puts-nam-config [format \
		"v -t %.15g -e sim_annotation %.15g $annotationSeq_ $str" \
		[$self now] [$self now] ]
}

proc trace_annotate { str } {
	set ns [Simulator instance]

	$ns trace-annotate $str
}

proc flash_annotate { start duration msg } {
	set ns [Simulator instance]
	$ns at $start "trace_annotate {$msg}"
	$ns at [expr $start+$duration] "trace_annotate periodic_message"
}

# rate's unit is second
Simulator instproc set-animation-rate { rate } {
	# time_parse defined in tcl/rtp/session-rtp.tcl
	set r [time_parse $rate]
	# This old nam api (set_rate) works but is quite obscure,
	# the new api (set_rate_ext) is simpler.
	# $self puts-nam-config "v -t [$self now] set_rate [expr 10*log10($r)] 1"
	$self puts-nam-config "v -t [$self now] set_rate_ext $r 1"
}
