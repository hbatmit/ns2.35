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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rtp/session-rtp.tcl,v 1.10 2000/08/18 18:34:05 haoboy Exp $
#

proc mvar args {
	upvar self _s
	uplevel $_s instvar $args
}

Session/RTP set uniq_srcid 0
Session/RTP proc alloc_srcid {} {
	set id [Session/RTP set uniq_srcid]
	Session/RTP set uniq_srcid [expr $id+1]
	return $id
}

Session/RTP instproc init {} {
	$self next 
	mvar dchan_ cchan_
	set cchan_ [new Agent/RTCP]
	set dchan_ [new Agent/CBR/RTP]
	$dchan_ set packetSize_ 512

	$dchan_ session $self
	$cchan_ session $self

	$self set rtcp_timer_ [new RTCPTimer $self]
	
	mvar srcid_ localsrc_
	set srcid_ [Session/RTP alloc_srcid]
	set localsrc_ [new RTPSource $srcid_]
	$self localsrc $localsrc_

	$self set srctab_ $localsrc_
	$self set stopped_ 1
}

Session/RTP instproc start {} {
	mvar group_
	if ![info exists group_] {
		puts "error: can't transmit before joining group!"
		exit 1
	}

	mvar cchan_ 
	$cchan_ start 
}

Session/RTP instproc stop {} {
	$self instvar cchan_ dchan_
	$dchan_ stop
	$cchan_ stop
	$self set stopped_ 1
}

Session/RTP instproc report-interval { i } {
	mvar cchan_
	$cchan_ set interval_ $i
}

Session/RTP instproc bye {} {
	mvar cchan_ dchan_
	$dchan_ stop
	$cchan_ bye
}

Session/RTP instproc attach-node { node } {
	mvar dchan_ cchan_
	global ns
	$ns attach-agent $node $dchan_
	$ns attach-agent $node $cchan_

	$self set node_ $node
}

Session/RTP instproc detach-node { node } {
	mvar dchan_ cchan_
	global ns
	$ns detach-agent $node $dchan_
	$ns detach-agent $node $cchan_

	$self unset node_
}

# Hook to enable easy syncronization with RTCP timeouts.
Session/RTP instproc rtcp_timeout {} {
	mvar rtcp_timeout_callback_
	
	if [info exists rtcp_timeout_callback_] {
		eval $rtcp_timeout_callback_
	}
}

Session/RTP instproc join-group { g } {
	set g [expr $g]

	$self set group_ $g

	mvar node_ dchan_ cchan_ 

	$dchan_ set dst_ $g
	$node_ join-group $dchan_ $g

	incr g

	$cchan_ set dst_ $g
	$node_ join-group $cchan_ $g
}

Session/RTP instproc leave-group { } {
	mvar group_ node_ cchan_ dchan_
	$node_ leave-group $dchan_ $group_
	$node_ leave-group $cchan_ [expr $group_+1]
	
	$self unset group_
}

Session/RTP instproc session_bw { bspec } {
	set b [bw_parse $bspec]

	$self set session_bw_ $b
    	
	mvar rtcp_timer_
	$rtcp_timer_ session-bw $b
}

Session/RTP instproc transmit { bspec } {
	set b [bw_parse $bspec]

	#mvar srcid_
	#global ns
	#puts "[$ns now] $self $srcid_ transmit $b"

	$self set txBW_ $b

	$self instvar dchan_ stopped_
	if { $b == 0 } {
		$dchan_ stop
		set stopped_ 1
	}

	set ps [$dchan_ set packetSize_]
	$dchan_ set interval_  [expr 8.*$ps/$b]
	if { $stopped_ == 1 } {
		$dchan_ start
		set stopped_ 0
	} else {
		$dchan_ rate-change
	}
}


Session/RTP instproc sample-size { cc } {
	mvar rtcp_timer_
	$rtcp_timer_ sample-size $cc
}

Session/RTP instproc adapt-timer { nsrc nrr we_sent } {
	mvar rtcp_timer_
	$rtcp_timer_ adapt $nsrc $nrr $we_sent
}

Session/RTP instproc new-source { srcid } {
	set src [new RTPSource $srcid]
	$self enter $src

	mvar srctab_
	lappend srctab_ $src

	return $src
}

Class RTCPTimer 

RTCPTimer instproc init { session } {
	$self next
	
	# Could make most of these class instvars.
	
	mvar session_bw_fraction_ min_rpt_time_ inv_sender_bw_fraction_
	mvar inv_rcvr_bw_fraction_ size_gain_ avg_size_ inv_bw_

	set session_bw_fraction_ 0.05

	# XXX just so we see some reports in short sim's...
	set min_rpt_time_ 1.   
	
	set sender_bw_fraction 0.25
	set rcvr_bw_fraction [expr 1. - $sender_bw_fraction]
	
	set inv_sender_bw_fraction_ [expr 1. / $sender_bw_fraction]
	set inv_rcvr_bw_fraction_ [expr 1. / $rcvr_bw_fraction]
	
	set size_gain_ 0.125	

	set avg_size_ 128.
	set inv_bw_ 0.

	mvar session_
	set session_ $session
	
        # Schedule a timer for our first report using half the
        # min ctrl interval.  This gives us some time before
        # our first report to learn about other sources so our
        # next report interval will account for them.  The avg
        # ctrl size was initialized to 128 bytes which is
        # conservative (it assumes everyone else is generating
        # SRs instead of RRs).
	
	mvar min_rtp_time_ avg_size_ inv_bw_
	set rint [expr 8*$avg_size_ * $inv_bw_]
	
	set t [expr $min_rpt_time_ / 2.]

	if { $rint < $t } {
		set rint $t
	}
	
	$session_ report-interval $rint
}
	
RTCPTimer instproc sample-size { cc } {
	mvar avg_size_ size_gain_

	set avg_size_ [expr $avg_size_ + $size_gain_ * ($cc + 28 - $avg_size_)]
}

RTCPTimer instproc adapt { nsrc nrr we_sent } {
	mvar inv_bw_ avg_size_ min_rpt_time_
	mvar inv_sender_bw_fraction_ inv_rcvr_bw_fraction_
	
	 # Compute the time to the next report.  we do this here
	 # because we need to know if there were any active sources
	 # during the last report period (nrr above) & if we were
	 # a source.  The bandwidth limit for ctrl traffic was set
	 # on startup from the session bandwidth.  It is the inverse
	 # of bandwidth (ie., ms/byte) to avoid a divide below.

	set ibw $inv_bw_
	if { $nrr > 0 } {
		if { $we_sent } {
			set ibw [expr $ibw * $inv_sender_bw_fraction_]
			set nsrc $nrr
		} else {
			set ibw [expr $ibw * $inv_rcvr_bw_fraction_]
			incr nsrc -$nrr
		}
	}
	
	set rint [expr 8*$avg_size_ * $nsrc * $ibw]	
	if { $rint < $min_rpt_time_ } {
		set rint $min_rpt_time_
	}
	
	mvar session_
	$session_ report-interval $rint
}
	
RTCPTimer instproc session-bw { b } {
	$self set inv_bw_ [expr 1. / $b ]
}

Agent/RTCP set interval_ 0.
Agent/RTCP set random_ 0
Agent/RTCP set class_ 32

RTPSource set srcid_ -1
