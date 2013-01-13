#
# Copyright (c) 1996 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/rlm/misc.tcl,v 1.1 1998/02/20 20:46:46 bajaj Exp $
#

proc uniform01 {} {
	return [expr double(([ns-random] % 10000000) + 1) / 1e7]
}

proc uniform { a b } {
	return [expr ($b - $a) * [uniform01] + $a]
}

proc exponential mean {
	return [expr - $mean * log([uniform01])]
}

proc trunc_exponential lambda {
	while 1 {
		set u [exponential $lambda]
		if { $u < [expr 4 * $lambda] } {
			return $u
		}
	}
}

proc sched { delay proc } {
	global sim
	return [$sim at [expr [$sim now] + $delay] $proc]
}

proc set_timer { which mmg delay } {
	global rlm_tid
	set v $which:$mmg
	if [info exists rlm_tid($v)] {
		puts "timer botched ($v)"
		exit -1
        }
	set rlm_tid($v) [sched $delay "trigger_timer $which $mmg"]
}

proc trigger_timer { which mmg } {
	global rlm_tid
	unset rlm_tid($which:$mmg)
	$mmg trigger_$which
}

#
# cancel s-timer on flow $mmg
# e.g., because we experienced loss
#
proc cancel_timer { which mmg } {
	global rlm_tid
	set v $which:$mmg
	if [info exists rlm_tid($v)] {
		#XXX cancel
		global sim
		$sim at $rlm_tid($v)
		unset rlm_tid($v)
	}
}

proc alloc_trace {} {
	global all_trace
	set t [new trace]
	lappend all_trace $t
	return $t
}

proc flush_all_trace {} {
	global all_trace
	if [info exists all_trace] {
		foreach t $all_trace {
			$t flush
		}
	}
}
