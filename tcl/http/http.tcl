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
# Code contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
#

proc rvValue {rv {op ""}} {
	if [catch "$rv value" val] {
		set val $rv
	}
	if {$op == ""} {
		return $val
	} else {
		return  [expr ${op}($val)]
	}
}


Class Http

Http set srcType_ TCP/Reno
Http set snkType_ TCPSink
Http set phttp_ 0
Http set maxConn_ 4

# The following rv* parameters can take a value or a RandomVariable object
Http set rvClientTime_ 3.0
Http set rvServerTime_ 0
Http set rvReqLen_ 256
Http set rvRepLen_ 4000
Http set rvImgLen_ 8000
Http set rvNumImg_ 4

Http instproc srcType {val} { $self set srcType_ $val }
Http instproc snkType {val} { $self set snkType_ $val }
Http instproc phttp {val} { $self set phttp_ $val }
Http instproc maxConn {val} { $self set maxConn_ $val }
Http instproc rvClientTime {val} { $self set rvClientTime_ $val }
Http instproc rvServerTime {val} { $self set rvServerTime_ $val }
Http instproc rvReqLen {val} { $self set rvReqLen_ $val }
Http instproc rvRepLen {val} { $self set rvRepLen_ $val }
Http instproc rvImgLen {val} { $self set rvImgLen_ $val }
Http instproc rvNumImg {val} { $self set rvNumImg_ $val }

Http proc setCDF {rvName value} {
	if [file exists $value] {
		set rv [new RandomVariable/Empirical]
		$rv loadCDF $value
		set value $rv
	}
	eval "$self set ${rvName}_ $value"
	return $value
}


Http instproc init {ns client server args} {
	eval $self init-vars $args
	$self instvar srcType_ snkType_ agents_ sinks_
	$self instvar phttp_ maxConn_ rvClientTime_ rvServerTime_
	$self instvar rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ numImg_ numGet_ numPut_ client_ server_ tStart_

	set ns_ $ns
	set client_(node) $client
	set server_(node) $server
	if {$phttp_} {
		set $maxConn_ 1
	}

	for {set i 0} {$i < $maxConn_} {incr i} {
		set csrc [new Agent/$srcType_]
		set ssrc [new Agent/$srcType_]
		lappend agents_(source) $csrc $ssrc
		$csrc proc done {} "$self doneRequest $i"
		$ssrc proc done {} "$self doneReply $i"
		if [string match "*FullTcp*" $srcType_] {
			set csnk $ssrc
			set ssnk $csrc
			$csrc set dst_ [$csnk set addr_]
			$csnk listen
		} else {
			set csnk [new Agent/$snkType_]
			set ssnk [new Agent/$snkType_]
			lappend agents_(sink) $csnk $ssnk
		}
		set client_($i) [$self newXfer FTP $client $server $csrc $csnk]
		set server_($i) [$self newXfer FTP $server $client $ssrc $ssnk]
		set sinks_($client_($i)) $csnk
		set sinks_($server_($i)) $ssnk
	}
}

Http instproc newXfer {type src dst sa da} {
	$self instvar ns_
	$ns_ attach-agent $src $sa
	$ns_ attach-agent $dst $da
	$ns_ connect $sa $da
	return [$sa attach-source $type]
}

# Http::agents return the agents of type "source" or "sink"
Http instproc agents {{type source}} {
	$self instvar agents_
	return $agents_($type)
}


Http instproc transmit {source nbyte {npkt 0}} {
	$self instvar numPut_ phttp_ sinks_ ns_ sessionLen_
	[$source set agent_] instvar packetSize_

#	puts "$source transmit $nbyte $npkt [$ns_ now]"
	if {$nbyte <= 0} {
		set nbyte 1000
	}
	if {$npkt == 1} {
		set packetSize_ $nbyte
	} else {
		set npkt [expr int(ceil(1.0 * $nbyte / $packetSize_))]
	}
	if {$phttp_ == 0} {
		[$source set agent_] reset
		$sinks_($source) reset
	}
	incr sessionLen_ $npkt
	$source producemore $npkt
}


Http instproc start {} {
	$self instvar phttp_ maxConn_ rvClientTime_ rvServerTime_
	$self instvar rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ numImg_ numGet_ numPut_ sessionLen_ \
			client_ server_ tStart_

	set tStart_ [$ns_ now]
	set numGet_ 0
	set numPut_ 0
	set sessionLen_ 0
	set len [rvValue $rvReqLen_ round]
	$self transmit $client_(0) $len 1

#	puts "Http $self starts at $tStart_"
#	puts "$self reqH$numGet_ [$client_(0) set agent_] $len"
}


Http instproc doneRequest {id} {
	$self instvar phttp_ maxConn_ rvClientTime_ rvServerTime_
	$self instvar rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ numImg_ numGet_ numPut_ client_ server_ tStart_ psz_

	if {$numPut_ == 0} {
		set len [rvValue $rvRepLen_ round]
	} else {
		set num [expr $phttp_ ? $numImg_ : 1]
		for {set i [set len 0]} {$i < $num} {incr i} {
			incr len [rvValue $rvImgLen_ round]
		}
	}

#	puts "$self reply [$server_($id) set agent_] $len [$ns_ now]"
	set cmd "$self transmit $server_($id) $len"
	set tServer [rvValue $rvServerTime_]
	if {$tServer > 0} {
		$ns_ at [expr $tServer+[$ns_ now]] $cmd
	} else {
		eval $cmd
	}
}


Http instproc doneReply {id} {
	$self instvar phttp_ maxConn_ rvClientTime_ rvServerTime_
	$self instvar rvReqLen_ rvRepLen_ rvNumImg_ rvImgLen_
	$self instvar ns_ numImg_ numGet_ numPut_ client_ server_ tStart_

	set idlist $id
	if {$numPut_ == 0} {
		set numImg_ [rvValue $rvNumImg_ round]
		for {set i 1} {$i < $maxConn_} {incr i} {
			lappend idlist $i
		}
	}
	if {[incr numPut_] > $numImg_} {
		$self donePage
		return
	}

	# set numGet_ and numPut_ as if only 1 more request is needed
	foreach id $idlist {
		if {[incr numGet_] > $numImg_} break
		if {$phttp_} {
			set numGet_ $numImg_
			set numPut_ $numImg_
		}
		set len [rvValue $rvReqLen_ round]
		$self transmit $client_($id) $len 1
#		puts "$self reqI$numGet_ $id [$client_($id) set agent_] $len [$ns_ now]"
	}
}

Http instproc donePage {} {
	$self instvar phttp_ maxConn_ rvClientTime_ rvServerTime_
	$self instvar ns_ numImg_ numGet_ numPut_ sessionLen_ \
			client_ server_ tStart_

	set now [$ns_ now]
	set ct [rvValue $rvClientTime_]
	set out [format "%.3f %.0f %d %.3f" [expr $now - $tStart_] [expr $numImg_+1] $sessionLen_ $ct]
	set curtime [format "%.3f" $now]
	# print "curtime transaction_time num_connections sessionlen thinktime"
	puts "$curtime Http $self donePage $out"
	$ns_ at [expr $now + $ct] "$self start"
}
