# Copyright (c) 1999 Regents of the University of California.
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
#       This product includes software developed by the MASH Research
#       Group at the University of California Berkeley.
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
# Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
#

if {![info exists ns]} {
	puts "Error: sat-teledesic-nodes.tcl is a supporting script for the "
	puts "       sat-teledesic.tcl script-- run `sat-teledesic.tcl' instead"
	exit
}

# 1. Configure initial classifier
# 2. Label plane that the satellite is in (used to identifying intraplane isl)
# 3. In each orbital plane, we need to "set_next" (set one-behind in orbit)
#      (this is a mandatory optimization to reduce handoff lookups)
for {set a 1} {$a <= 12} {incr a} {
	for {set i [expr $a * 100]} {$i < [expr $a * 100 + 24]} {incr i} {
		if {$a % 2} {
			set stagger 0
		} else { 
			set stagger 7.5 
		}
		set imod [expr $i % 100]
		set n($i) [$ns node]
		$n($i) set-position $alt $inc [expr 15 * ($a - 1)] \
		    [expr $stagger + $imod * 15] $a 
	}
}
# Set up next pointer
for {set a 1} {$a <= 12} {incr a} {
	for {set i [expr $a * 100]} {$i < [expr $a * 100 + 24]} {incr i} {
		if {$i % 100} {
			set next [expr $i - 1]
		} else {
			set next [expr $a * 100 + 23]
		}	
		$n($i) set_next $n($next)
	}
}

