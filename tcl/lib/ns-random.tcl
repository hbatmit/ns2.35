#
# Copyright (c) 1996-1997 Regents of the University of California.
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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-random.tcl,v 1.14 1999/10/09 01:06:41 haoboy Exp $
#

#Code to generate random numbers here
proc exponential {args} {
	global defaultRNG
	eval [list $defaultRNG exponential] $args
}

proc uniform {args} {
	global defaultRNG
	eval [list $defaultRNG uniform] $args
}

proc integer {args} {
	global defaultRNG
	eval [list $defaultRNG integer] $args
}

RNG instproc init {} {
        $self next
        #z2 used to create a normal distribution  from a uniform distribution
        #using the polar method
        $self instvar z2
        set z2 0
}

RNG instproc uniform {a b} {
	expr $a + (($b - $a) * ([$self next-random] * 1.0 / 0x7fffffff))
}

RNG instproc integer k {
	expr [$self next-random] % abs($k)
}

RNG instproc exponential {{mu 1.0}} {
	expr - $mu * log(([$self next-random] + 1.0) / (0x7fffffff + 1.0))
}

#RNG instproc normal {a1 a2 } {
#        $self instvar z2
#        if {$z2 !=0 } {
#                set z1 $z2
#                set z2 0
#        } else {
#                set w 1
#                while { $w >= 1.0 } {
#                        set v1 [expr 2.0 * ([$self next-random] *1.0/0x7fffffff) - 1.0]
#                        set v2 [expr 2.0 * ([$self next-random] *1.0/0x7fffffff) - 1.0]
#                        set w  [expr ($v1*$v1+$v2*$v2)]
#                }
#                set w [expr  sqrt((-2.0*log($w))/$w)]
#                set z1 [expr $v1*$w]
#                set z2 [expr $v2*$w]
#        }
#        expr $a1 + $z1 *$a2
#}

#RNG instproc lognormal {med stddev } {
#        expr $med *exp($stddev*[$self normal 0.0 1.0])
#}

RandomVariable instproc test count {
	for {set i 0} {$i < $count} {incr i} {
		puts stdout [$self value]
	}
}


set defaultRNG [new RNG]
$defaultRNG seed 1
$defaultRNG default
#
# Because defaultRNG is not a bound variable but is instead
# just copied into C++, we enforce this.
# (A long-term solution be to make defaultRNG bound.)
# --johnh, 30-Dec-97
#
trace variable defaultRNG w { abort "cannot update defaultRNG once assigned"; }


# Trace driven random variable.
# Polly Huang, March 4 1999
Class RandomVariable/TraceDriven -superclass RandomVariable

RandomVariable/TraceDriven instproc init {} {
    $self instvar filename_ file_
}

RandomVariable/TraceDriven instproc value {} {
    $self instvar file_ filename_

    if ![info exist file_] {
        if [info exist filename_] {
            set file_ [open $filename_ r]
        } else {
            puts "RandomVariable/TraceDriven: Filename is not given"
            exit 0
        }
    }

    if ![eof $file_] {
        gets $file_ tmp
        return $tmp
    } else {
        close $file_
        puts "Error: RandomVariable/TraceDriven: Reached the end of the trace fi
le "
        exit 0
    }
}
