#
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

if {![info exists ns]} {
	puts "Error: sat-teledesic-links.tcl is a supporting script for the "
	puts "       sat-teledesic.tcl script-- run `sat-teledesic.tcl' instead"
	exit
}

# Now that the positions are set up, configure the ISLs
# Intraplane
for {set a 1} {$a <= 12} {incr a} {
	for {set i [expr $a * 100]} {$i < [expr $a * 100 + 24]} {incr i} {
		set imod [expr $i % 100]
		if {$imod == 23} {
			set next [expr $a * 100]
		} else {
			set next [expr $i + 1]
		}
		if {$imod == 23} {
			set next2 [expr $a * 100 + 1]
		} elseif {$imod == 22} {
			set next2 [expr $a * 100]
		} else {
			set next2 [expr $i + 2]
		}
		$ns add-isl intraplane $n($i) $n($next) $opt(bw_isl) $opt(ifq) $opt(qlim)
		$ns add-isl intraplane $n($i) $n($next2) $opt(bw_isl) $opt(ifq) $opt(qlim)
	}
}

# Interplane ISLs
# 4 interplane ISLs per satellite 

for {set i 100} {$i < 124} {incr i} {
	set next [expr $i + 100]
	$ns add-isl interplane $n($i) $n($next) $opt(bw_isl) $opt(ifq) $opt(qlim)
}

for {set a 3} {$a <= 12} {incr a} {
	for {set i [expr $a * 100]} {$i < [expr $a * 100 + 24]} {incr i} {
		set prev [expr $i - 100]
		set prev2 [expr $i - 200]
		$ns add-isl interplane $n($i) $n($prev) $opt(bw_isl) $opt(ifq) $opt(qlim)
		$ns add-isl interplane $n($i) $n($prev2) $opt(bw_isl) $opt(ifq) $opt(qlim)
	}
}

# Crossseam ISLs
# Plane 1 <-> Plane 12
for {set i 100} {$i < 112} {incr i} {
	set j [expr 1311 - $i]
	$ns add-isl crossseam $n($i) $n($j) $opt(bw_isl) $opt(ifq) $opt(qlim)
}
for {set i 112} {$i < 124} {incr i} {
	set j [expr 1335 - $i]
	$ns add-isl crossseam $n($i) $n($j) $opt(bw_isl) $opt(ifq) $opt(qlim)
}
