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
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/lib/ns-nam.tcl,v 1.3 1997/07/24 21:18:58 heideman Exp $
#
Class NamSimulator -superclass Simulator

NamSimulator instproc init { outfile } {
	$self next
	$self set chan_ [open $outfile w]
	
	$self set nam_config_ "proc nam_config {net} \{\n"
}


NamSimulator instproc node {} {
	set n [$self next]

	set namcmd "\t\$net node [$n id] circle\n"
	$self instvar nam_config_
	set nam_config_ $nam_config_$namcmd
	
	return $n
}

NamSimulator instproc simplex-link { n1 n2 bw delay type } {
	$self next $n1 $n2 $bw $delay $type
	set id1 [$n1 id]
	set id2 [$n2 id]

	$self instvar nam_config_ link_mark_
	if { ![info exists link_mark_($id2,$id1)] } {
		set namcmd "\tauto_mklink \$net $id1 $id2 $bw $delay \n"
		set nam_config_ $nam_config_$namcmd
	}

	set link_mark_($id1,$id2) 1

	set namcmd "\t\$net queue $id1 $id2 0.5 \n"
	set nam_config_ $nam_config_$namcmd
}


NamSimulator instproc config_dump {} {
	$self instvar nam_config_
	set nam_config_ $nam_config_\}\n
	
	$self instvar chan_
	puts $chan_ $nam_config_
	close $chan_
}
