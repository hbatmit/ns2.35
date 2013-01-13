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
# @(#) $Header: /cvsroot/nsnam/ns-2/bin/tcl-expand.tcl,v 1.4 2005/05/30 05:25:14 sfloyd Exp $
#

#
# copy files on command line to stdout and substitute files
# of source commands.  only works for simple, literal commands.
#

proc expand_file name {
	puts "### tcl-expand.tcl: begin expanding $name"
	if {[file exists $name] == 0} {
		puts "### tcl-expand.tcl: cannot find $name"
		return 
	}
	set f [open $name r]
	while 1 {
		if { [gets $f line] < 0 } {
			close $f
			puts "### tcl-expand.tcl: end expanding $name"
			return
		}
		set L [split $line]
		if { [llength $L] == 2 && [lindex $L 0] == "source" } {
			expand_file [lindex $L 1]
		} else {
			puts $line
		}
	}
}

set startupDir [pwd]
fconfigure stdout -translation lf
foreach name $argv {
	set dirname [file dirname $name]
	if {$dirname != "."} {
		cd $dirname
		expand_file [file tail $name]
		cd $startupDir
	} else {
		expand_file $name
	}
}

exit 0
