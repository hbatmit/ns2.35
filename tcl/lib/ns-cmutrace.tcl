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
#
# $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-cmutrace.tcl,v 1.3 2003/09/27 01:01:31 aditi Exp $

CMUTrace instproc init { tname type } {
	$self next $tname $type
	$self instvar type_ src_ dst_ callback_ show_tcphdr_

	set type_ $type
	set src_ 0
	set dst_ 0
	set callback_ 0
	set show_tcphdr_ 0
}

CMUTrace instproc attach fp {
	$self instvar fp_
	set fp_ $fp
	$self cmd attach $fp_
}

Class CMUTrace/Send -superclass CMUTrace
CMUTrace/Send instproc init { tname } {
	$self next $tname "s"
}

Class CMUTrace/Recv -superclass CMUTrace
CMUTrace/Recv instproc init { tname } {
	$self next $tname "r"
}

Class CMUTrace/Drop -superclass CMUTrace
CMUTrace/Drop instproc init { tname } {
	$self next $tname "D"
}

Class CMUTrace/EOT -superclass CMUTrace
 CMUTrace/EOT instproc init { tname } {
       $self next $tname "x"
 }

# XXX MUST NOT initialize off_*_ here!! 
# They should be automatically initialized in ns-packet.tcl!!

CMUTrace/Recv set src_ 0
CMUTrace/Recv set dst_ 0
CMUTrace/Recv set callback_ 0
CMUTrace/Recv set show_tcphdr_ 0

CMUTrace/Send set src_ 0
CMUTrace/Send set dst_ 0
CMUTrace/Send set callback_ 0
CMUTrace/Send set show_tcphdr_ 0

CMUTrace/Drop set src_ 0
CMUTrace/Drop set dst_ 0
CMUTrace/Drop set callback_ 0
CMUTrace/Drop set show_tcphdr_ 0

CMUTrace/EOT set src_ 0
 CMUTrace/EOT set dst_ 0
 CMUTrace/EOT set callback_ 0
 CMUTrace/EOT set show_tcphdr_ 0

