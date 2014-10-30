#
# Copyright (c) 1997 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rtp/session-scuba.tcl,v 1.4 1997/11/29 05:42:53 elan Exp $
#

Class Session/RTP/Scuba -superclass Session/RTP

Session/RTP/Scuba instproc init {} {
	$self next
	
	$self instvar repAgent_ cchan_
	set repAgent_ [new Agent/Message/Scuba]
    	$repAgent_ set session_ $self

	$self set reptimer_ [new Timer/Scuba $self timeout 0]
	$self set agetimer_ [new Timer $self age_scores]

	$self set localrepid_ 0
	$self set prep_ {}
	$self set share_ 0.05
	$self set scuba_srctab_ {}
	$self set tx_ 0
}

Session/RTP/Scuba instproc start { tx rx } {
    	$self next
	$self instvar reptimer_ agetimer_ tx_ srcid_
	if { $rx == 1 } {
		$reptimer_ start
	}

	set tx_ $tx  
  	if { $tx_ == 1 } {
		$self scuba_register $srcid_ 0
		$self set_allocation 
		$agetimer_ msched 5000
	}
}

Session/RTP/Scuba instproc session_bw { bspec } {
	#XXX Should pass on 0.95*session_bw
	$self next $bspec

	set b [bw_parse $bspec]

	$self instvar reptimer_
	$reptimer_ set ctrl_bw_ [expr 0.05*$b]
}


Session/RTP/Scuba instproc attach-node { node } {
    	$self next $node
    	
	global ns 
    	$self instvar repAgent_
	$ns attach-agent $node $repAgent_
    	$repAgent_ set node $node
}

Session/RTP/Scuba instproc detach-node { node } {
    	$self next $node
    	
	global ns 
    	$self instvar repAgent_
	$ns detach-agent $node $repAgent_
    	$repAgent_ unset node
}

Session/RTP/Scuba instproc join-group { g } {
    	$self next $g
    	
    	$self instvar repAgent_ node_

    	set g [expr $g+2]
	$repAgent_ set dst_ $g
	$node_ join-group $repAgent_ $g
}
 
Session/RTP/Scuba instproc leave-group { } {
    	$self next

    	$self instvar_ node_ group_ repAgent_
	$node_ leave-group $repAgent_ [expr $group_+2]
}

Session/RTP/Scuba instproc timeout {} {
	$self instvar localrepid_ repAgent_ srcid_ rx_
	set rep [$self build_report]
	$repAgent_ send "$srcid_/$localrepid_/$rep"
	incr localrepid_
	
	$self instvar scuba_srctab_ reptimer_
	set nsrcs [llength $scuba_srctab_]
	set rint [$reptimer_ adapt $nsrcs]

	#XXX
	if { $rint < 0.5 } {
		set rint 0.5
	}

	$reptimer_ msched $rint
}

Session/RTP/Scuba instproc scuba_register { sender repid } {
	# Add new source if we hear a ctrl message from it as well
	$self instvar scuba_srctab_ last_repid_ agetab_

	if { [lsearch -exact $scuba_srctab_ $sender] < 0 } {
		lappend scuba_srctab_ $sender
	}

	# XXX get rid of repid_...
	set last_repid_($sender) $repid
	set agetab_($sender) 0
}

Session/RTP/Scuba instproc recv_priority_report { sender repid rep } {
	$self scuba_register $sender $repid
	
#puts "$self: $proc $sender/$repid/$rep"

	foreach e $rep {
		set srcid [lindex $e 0]
		set val [lindex $e 1]
		$self recv_scuba_entry $sender $repid $srcid $val
	}
	$self clean_scoretab $sender $repid
	$self instvar tx_
	if { $tx_ == 1 } {
		$self set_allocation
	}
}

Session/RTP/Scuba instproc recv_scuba_entry { sender repid srcid val } {
	$self instvar scoretab_ agetab_

#puts "$self: $proc $sender/$repid/$srcid/$val"

	set scoretab_($sender:$srcid:$repid) [expr $val/1e6]
	set agetab_($sender) 0
}

Session/RTP/Scuba instproc clean_scoretab { sender repid } {
	$self instvar scoretab_ agetab_

	set idxs [array names scoretab_ $sender:*]
	
	foreach i $idxs {
		set r [split $i :]
		set r [lindex $r 2]
		if { $r < $repid } {
			unset scoretab_($i)
		}
	}
}

Session/RTP/Scuba instproc set_allocation {} {
	$self instvar scoretab_ scuba_srctab_ share_ srcid_

	set lsrcid $srcid_
	
	#
	# Tabulate scores
	#
	# For now, just find ourselves in the score table and allocate
	# our bandwith proportionally.  If our bandwidth is 0, we get a 
	# proportional fraction of 5% of the bandwidth which is set aside for
	# this purpose.

	set total 0
	set tot($lsrcid) 0
	$self instvar srctab_
	foreach src $srctab_ {
		set srcid [$src set srcid_]
		set voters [array names scoretab_ *:$srcid:*]
		set subtotal 0
		foreach v $voters {
			set subtotal [expr $subtotal+$scoretab_($v)]
		}
		set tot($srcid) $subtotal
		set total [expr $total+$subtotal]
	}
#puts "total=$total localtot=$tot($lsrcid)"
	if { $total > 0 } {
		set avg [expr $tot($lsrcid)/$total]
	} else {
		set avg 0
	}
	if { $avg > 0 } {
		# 5% for rtcp, 5% scuba
		set share_ [expr 0.90*$avg]
	} else {
		$self instvar srctab_
		set nsrcs [llength $srctab_]
		if { $nsrcs == 0 } {
			set nsrcs 1
		}
		set share_ [expr 0.05/$nsrcs]
	}

	$self instvar session_bw_

global ns
#puts "[$ns now] $self: $lsrcid set_bps $share_ of $session_bw_=[expr $share_*$session_bw_]"
	$self set_bps [expr $share_*$session_bw_]
}

Session/RTP/Scuba instproc set_bps { bps } {
	$self transmit $bps
}


Session/RTP/Scuba instproc build_report {} {
	$self instvar focus_set_ localrepid_ srcid_
	if ![info exists focus_set_] {
		return {}
	}
	set rep {}

	set localsrc $srcid_

	# Divvy up score equally among all in focus set
	set t 0
	set srcs [array names focus_set_]
	foreach s $srcs {
		# focus_set is 0-1 valued.
		# Ignore our own focus
		if { [$s set srcid_] != $localsrc } {
			set t [expr $t+$focus_set_($s)]
		}
	}

	set lid $localrepid_
	# Loopback our report
	$self scuba_register $localsrc $lid

	if { $t != 0 } {
		set score [expr int(1e6/$t)]
		foreach s $srcs {
			if { $focus_set_($s) != 0 } {
				set srcid [$s set srcid_]

				lappend rep "$srcid $score"

				# Loopback our votes, but ignore our own.
				if {[$s set srcid_] != $localsrc} {
					$self recv_scuba_entry $localsrc $lid \
							$srcid $score
				}
			}
		}
	}
	# Complete loopback with cleanup and reallocation
	$self clean_scoretab $localsrc $lid
	
	$self instvar tx_
	if { $tx_ == 1 } {
		$self set_allocation
	}
	
	return $rep
}

Session/RTP/Scuba instproc scuba_focus { src } {
#puts "focus $src"
	$self set focus_set_($src) 1
}

Session/RTP/Scuba instproc scuba_unfocus { src } {
#puts "unfocus $src"
	$self set focus_set_($src) 0
}

Session/RTP/Scuba instproc age_scores { } {
	$self instvar agetab_ last_repid_ agetimer_
	if ![info exists agetab_] {
		$agetimer_ msched 5000
		return
	}
	set senders [array names agetab_]
	
	# For now, if we haven't heard from you in 30 seconds - we ignore the
	# entries.
	# What we want to do is have a sliding window.  Get to that later.
	set age_thresh 6

	set localsrc [$self set srcid_]
	foreach s $senders {
		if { $s == $localsrc } {
			continue
		}
		incr agetab_($s)
		if { $agetab_($s) > $age_thresh } {
			$self delete_sender $s
		}
	}
	$agetimer_ msched 5000
}


Session/RTP/Scuba instproc delete_sender { s } {
	$self instvar last_repid_ agetab_ scuba_srctab_

	$self clean_scoretab $s [expr $last_repid_($s)+1]

	unset agetab_($s)

	set i [lsearch -exact $scuba_srctab_ $s]
	set scuba_srctab_ [lreplace $scuba_srctab_ $i $i]

	$self instvar tx_
	if { $tx_ == 1 } {
		$self set_allocation
	}
}

Class Agent/Message/Scuba -superclass Agent/Message

Agent/Message/Scuba instproc handle { report } {
	set R [split $report /]
	
	set sender [lindex $R 0]
	set repid [lindex $R 1]
	set rep [lindex $R 2]
	
    	$self instvar session_
	$session_ recv_priority_report $sender $repid $rep
}

Agent/Message/Scuba set class_ 33
Agent/Message/Scuba set packetSize_ 52

Class Timer

Timer instproc init { manager callback } {
	$self next 
	$self set callback_ $callback
	$self set manager_ $manager
}	

Timer instproc msched { t } {
	global ns
	$self instvar id_ manager_ callback_
	
	if [info exists id_] {
		puts stderr "warning: $self ($class): overlapping timers."
	}
	
	set id_ [$ns at [expr [$ns now]+$t/1000.] "$self timeout"]
}
	
Timer instproc timeout {} {
	$self instvar id_ manager_ callback_
	unset id_
	
	eval $manager_ $callback_
}

Timer instproc cancel {} {
	global ns
	$self instvar id_
	if [info exists id_] {
		$ns cancel $id_
		unset id_
	}
}

Class Timer/Scuba -superclass Timer

Timer/Scuba instproc init { manager callback rand } {
	$self next $manager $callback

	$self set avgsize_ [Agent/Message/Scuba set packetSize_]
	$self set randomize_ $rand
	ns-random 0
}

Timer/Scuba instproc adapt { nsrcs } {
	$self instvar ctrl_bw_ avgsize_ randomize_

	set t [expr 1000*($nsrcs*$avgsize_*8)/$ctrl_bw_]
	
	if { $randomize_ != 0 } {
		# Random number in U[-0.5,0.5]
		set r [expr [ns-random]/double(0x7fffffff)-0.5]
		set t [expr $t+$t*$r]
	}

	return $t
}

Timer/Scuba instproc start { } {
	$self msched [$self adapt 1]
}
