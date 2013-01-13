# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Time-stamp: <2000-09-11 14:54:38 haoboy>
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
#  Original source contributed by Gaeil Ahn. See below.
#
#  $Header: /cvsroot/nsnam/ns-2/tcl/mpls/ns-mpls-ldpagent.tcl,v 1.3 2005/09/16 03:05:45 tomh Exp $

###########################################################################
# Copyright (c) 2000 by Gaeil Ahn                                	  #
# Everyone is permitted to copy and distribute this software.		  #
# Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     #
# this sources.								  #
###########################################################################

#############################################################
#                                                           #
#     File: File for LDP protocol                           #
#     Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      #
#                                                           #
#############################################################

Agent/LDP instproc set-mpls-module { mod } {
	$self set module_ $mod
}

Agent/LDP instproc get-request-msg {msgid src fec pathvec} {
	$self instvar node_ module_

	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$module_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "LoopDetected" -1           
		
		return
	}
	
	set nexthop [$module_ get-nexthop $fec]
	if {$src == $nexthop} {
		#  message from downstream node
		$self request-msg-from-downstream $msgid $src $fec $pathvec
	} else {
		#  message from upstream node
		$self request-msg-from-upstream $msgid $src $fec $pathvec
	}
}

Agent/LDP instproc request-msg-from-downstream {msgid src fec pathvec} {
	$self instvar module_

	#  1. allocate label (upstream label allocation)
	set outlabel [$module_ get-outgoing-label $fec -1]
	if { $outlabel < 0 } {
		if { $fec == $src } {
			# This is a penultimate hop
			set outlabel 0
		} else {
			set outlabel [$module_ new-outgoing-label]
		}     
		$module_ out-label-install $fec -1 $src $outlabel             
	} else {
		set outIface [$module_ get-outgoing-iface $fec -1]
		if { $src != $outIface} {
			# wrong LIB information, let's fix it up
			$module_ out-label-install $fec -1 $src $outlabel
		} 
	}
	
	#  2. reply a allocated label to $src
	set ldpagent [$module_ get-ldp-agent $src]
	$ldpagent new-msgid
	$ldpagent send-mapping-msg $fec $outlabel "*" $msgid

	#  3. trigger label distribution (control-driven-trigger)
	#     not support on-demand mode in control-driven-trigger
	$module_ ldp-trigger-by-control $fec $pathvec
}

Agent/LDP instproc request-msg-from-upstream {msgid src fec pathvec} {
	$self instvar module_

	#  ldp agent for src
	set ldpagent [$module_ get-ldp-agent $src]

	# This is a egress LSR
	if { [$module_ is-egress-lsr $fec] == 1 } {
                $ldpagent new-msgid
                $ldpagent send-mapping-msg $fec 0 "*" $msgid
                return
	}
	
	set inlabel  [$module_ get-incoming-label $fec -1]
	set outlabel [$module_ get-outgoing-label $fec -1]
	#
	# ordered-control mode
	#
	if { [Classifier/Addr/MPLS ordered-control?] == 1 } {
		if { $outlabel > -1 } {
			if { $inlabel < 0 } {
				set inlabel [$module_ new-incoming-label]
			}
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec inlabel "*" $msgid
		} else {
			$module_ ldp-trigger-by-data $msgid $src $fec $pathvec
		}
		return
	}
	#
	#  independent-control mode
	#
	#  1. allocate label (upstream label allocation)
	if { $inlabel < 0 } {
		set inlabel [$module_ new-incoming-label]
		$module_ in-label-install $fec -1 $src $inlabel
	} else {
		set inIface [$module_ get-incoming-iface $fec -1]
		if { $src != $inIface} {
			set classifier [$module_ set classifier_]
			set dontcare [$classifier set dont_care_]
			$module_ in-label-install $fec -1 -1 $dontcare
		} 
	}
	#  2. reply a allocated label to $src
	$ldpagent new-msgid
	$ldpagent send-mapping-msg $fec $inlabel "*" $msgid
	#  3. trigger label distribution (data-driven-trigger)
	#     support on-demand mode in data-driven-trigger
	$module_ ldp-trigger-by-data $msgid $src $fec $pathvec
}

Agent/LDP instproc get-cr-request-msg {msgid src fec pathvec er lspid rc} {
	$self instvar node_ module_

	#
	#  CR-LDP:  used to setup explicit-LSP
	#
	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$module_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "NoRoute" $lspid
		return
	}
	
	# reserve a required resources using $rc
	# if a reservation fails, send notificaiton-msg

	#  ldp agent for src
	set ldpagent [$module_ get-ldp-agent $src]

	#  if a outgoing label for the fec exists ingnore this request-msg and
	#  send cr-mapping-msg.
	set inlabel [$module_ get-incoming-label $fec $lspid]
	set outlabel [$module_ get-outgoing-label $fec $lspid]
	
	if { $outlabel > -1 } {
                if { $inlabel < 0 } {
			set inlabel [$module_ new-incoming-label]
			$module_ in-label-install $fec $lspid $src $inlabel
                }
                $ldpagent new-msgid
                $ldpagent send-cr-mapping-msg $fec $inlabel $lspid $msgid
                return
	}

	$module_ ldp-trigger-by-explicit-route $msgid $src $fec $pathvec $er \
			$lspid $rc
}

Agent/LDP instproc get-cr-mapping-msg {msgid src fec label lspid reqmsgid} {
	$self instvar node_ trace_ldp_ module_

	# select a upstream-node to receive a mapping-msg in msgtbl
	# and clear the msgtbl entry

	set prvsrc   [$self msgtbl-get-src       $reqmsgid]
	set prvmsgid [$self msgtbl-get-reqmsgid  $reqmsgid]
	set labelop  [$self msgtbl-get-labelop   $reqmsgid]
	if {$labelop == 2} {
		set tunnelid [$self msgtbl-get-erlspid   $reqmsgid]
	} else {
		set tunnelid -1
	}
	
	$self msgtbl-clear $reqmsgid

	if { $trace_ldp_ } {
		puts "$src -> [$node_ id] : prvsrc($prvsrc)"
	}
	
	# allocate outgoing-label
	if { $labelop == 2 } {
		# stack operation
		$module_ out-label-install $fec $lspid $src $label
		$module_ erlsp-stacking $lspid $tunnelid
	} elseif {$labelop == 1} {
		# only pass label
		set ldpagent [$module_ get-ldp-agent $prvsrc]
		$ldpagent new-msgid
		$ldpagent send-cr-mapping-msg $fec $label $lspid $prvmsgid
		return
	} else {
		# use the label
		$module_ out-label-install $fec $lspid  $src $label
	}
	
	if {$prvsrc == [$node_ id]} {
		return
	}

	# allocate incoming-label            
	set inlabel [$module_ new-incoming-label]
	$module_ in-label-install $fec $lspid $prvsrc $inlabel

	# find a ldpagent for a upstream-node, continually send a 
	# mapping-msg to the node
	set ldpagent [$module_ get-ldp-agent $prvsrc]
	$ldpagent new-msgid
	$ldpagent send-cr-mapping-msg $fec $inlabel $lspid $prvmsgid
}

# msgid    : the msgid of a mapping message sent by dstination
# reqmsgid : the msgid of a request message sent by me before
Agent/LDP instproc get-mapping-msg {msgid src fec label pathvec reqmsgid} {
	$self instvar node_ trace_ldp_ module_

	if { $trace_ldp_ } {
		puts "[[Simulator instance] now]: <mapping-msg> $src ->\
[$node_ id] : fec($fec), label($label) [$module_ get-nexthop $fec]"
	}
	
	set pathvec [split $pathvec "_"]
	if {[lsearch $pathvec [$node_ id]] > -1} {
		# loop detected
		set ldpagent [$module_ get-ldp-agent $src]
		$ldpagent new-msgid
		$ldpagent send-notification-msg "LoopDetected" -1           
		return
	}

	set nexthop [$module_ get-nexthop $fec]
	if {$src == $nexthop} {
		#  message from downstream node
		$self mapping-msg-from-downstream $msgid $src $fec $label \
				$pathvec $reqmsgid
	} else {
		#  message from upstream node
		$self mapping-msg-from-upstream $msgid $src $fec $label \
				$pathvec $reqmsgid
	}
}

Agent/LDP instproc mapping-msg-from-downstream {msgid src fec label \
		pathvec reqmsgid} {
	$self instvar node_ module_

	#  allocate label (upstream label allocation)
	$module_ out-label-install $fec -1 $src $label
	if { $reqmsgid > -1 } {
		# downstream-on-demand mode
		
		# 1. find a upstream-node who sent a request-msg in msgtbl
		#    & delete entry in msgtbl
		set prvsrc   [$self msgtbl-get-src      $reqmsgid]
		set prvmsgid [$self msgtbl-get-reqmsgid $reqmsgid]
		$self msgtbl-clear $reqmsgid
		if { $prvsrc == [$node_ id] || $prvsrc < 0} {
			return
		}
		if { [Classifier/Addr/MPLS ordered-control?] == 1 } {
			# ordered-control mode

			# 1. allocate incoming-label
			set inlabel [$module_ new-incoming-label]
			$module_ in-label-install $fec -1 $prvsrc $inlabel
			# 2. find a ldpagent for a upstream-node,
			#    send a mapping-msg forward a upstream-node
			set ldpagent [$module_ get-ldp-agent $prvsrc]
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $inlabel -1 $prvmsgid
			return
		}
	} else {
		#  trigger label distribution (control-driven-trigger)
		$module_ ldp-trigger-by-control $fec $pathvec
		return
	}
}

Agent/LDP instproc mapping-msg-from-upstream {msgid src fec label pathvec \
		reqmsgid} {
	$self instvar node_ module_

	# check whether or not myself is a next hop of src for a fec.
	set nexthop [$module_ lookup-nexthop $src $fec]
	if { $nexthop != [$node_ id] } {
		return
	}

	#  1. allocate label (upstream label allocation)
	set inlabel [$module_ get-incoming-label $fec -1]
	if { $inlabel == -1 } {
		if { [$module_ is-egress-lsr $fec] == 1 } {
			#  message from a penultimate hop
			#  so, send a null label to a penultimate hop
			if { $label != 0 } {
				set ldpagent [$module_ get-ldp-agent $src]
				$ldpagent new-msgid
				$ldpagent send-mapping-msg $fec 0 "*" $msgid
			}
		} else {
			$module_ in-label-install $fec -1 $src $label
		}
	} else {
		set ldpagent [$module_ get-ldp-agent $src]
		if { $reqmsgid < 0 } {
			$ldpagent new-msgid
			$ldpagent send-mapping-msg $fec $inlabel "*" $msgid
		}   
	}    
	#  2. trigger label distribution (data-driven-trigger)
	#     support on-demand mode only in data-driven-trigger
	if { $reqmsgid < 0 } {
		$module_ ldp-trigger-by-data -1 $src $fec $pathvec
	} else {   
		# if ($reqmsgid > -1) then delete a entry in msgid table
		$self msgtbl-clear $reqmsgid
	}    
}

Agent/LDP instproc get-notification-msg {src status lspid} {
	$self instvar node_ trace_ldp_ module_

	# Notification-msg (in case of Loop Detection, No Route)
	if { $trace_ldp_ } {
		puts "Notification($src->[$node_ id]): $status src=$src lspid=$lspid"
	}
	set msgid [$self msgtbl-get-msgid -1 $lspid -1]
	if {$msgid > -1} {
		set prvsrc   [$self msgtbl-get-src      $msgid]
		$self msgtbl-clear $msgid            
		if { $prvsrc < -1 || $prvsrc == [$node_ id] } {
			return
		}
		set ldpagent [$module_ get-ldp-agent $prvsrc]
		$ldpagent new-msgid
		$ldpagent send-notification-msg $status $lspid
	}
}

Agent/LDP instproc get-withdraw-msg {src fec lspid} {
	$self instvar module_

	# withdraw msg : downstream -> upstream
	set outiface  [$module_ get-outgoing-iface $fec $lspid]
	if {$src == $outiface} {
		$module_ out-label-clear $fec $lspid
		set inlabel [$module_ get-incoming-label $fec $lspid]
		if {$inlabel > -1} {
			$module_ ldp-trigger-by-withdraw $fec $lspid
		}
	}
}

Agent/LDP instproc get-release-msg {src fec lspid} {
	$self instvar module_
	
	# release msg : upstream --> downstream
	set iniface  [$module_ get-incoming-iface $fec $lspid]
	set outlabel [$module_ get-outgoing-label $fec $lspid]
	if {$iniface == $src} {
		$module_ in-label-clear $fec $lspid 
		if {$outlabel > -1} {
			$module_ ldp-trigger-by-release $fec $lspid
		}
	} 
}

Agent/LDP instproc trace-ldp-packet {src_addr src_port msgtype msgid fec \
		label pathvec lspid er rc reqmsgid status atime} {
	$self instvar node_
	puts "$atime [$node_ id]: $src_addr ($msgtype $msgid) $fec $label $pathvec  \[$reqmsgid $status\]  \[$lspid $er $rc\]"
}

Agent/LDP instproc send-notification-msg {status lspid} {
	$self set fid_ 101
	$self cmd notification-msg $status $lspid
}

Agent/LDP instproc send-request-msg {fec pathvec} {
	$self set fid_ 102
	$self request-msg $fec $pathvec
}

Agent/LDP instproc send-mapping-msg {fec label pathvec reqmsgid} {
	$self set fid_ 103
	$self cmd mapping-msg $fec $label $pathvec $reqmsgid
}

Agent/LDP instproc send-withdraw-msg {fec lspid} {
	$self set fid_ 104
	$self withdraw-msg $fec $lspid
}

Agent/LDP instproc send-release-msg {fec lspid} {
	$self set fid_ 105
	$self release-msg $fec $lspid
}

Agent/LDP instproc send-cr-request-msg {fec pathvec er lspid rc} {
	$self set fid_ 102
	$self cr-request-msg $fec $pathvec $er $lspid $rc
}

Agent/LDP instproc send-cr-mapping-msg {fec inlabel lspid prvmsgid} {
	$self set fid_ 103
	$self cr-mapping-msg $fec $inlabel $lspid $prvmsgid
}

