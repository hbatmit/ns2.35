#!/usr/bin/tclsh

#
# Copyright (C) 2001 by USC/ISI
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# An tcl script that extracts DATA/ACK packets from/to ISI domain, 
# used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

if { $argc != 1} {
	puts "usage: io.tcl <tcpdump ASCII file> "
	exit
} else {
	set arg [split $argv " " ]
	set tfile [lindex $arg 0]
}

set isiPrefix "129.1"

set fi [open $tfile r ]
set f1 [open $tfile.inbound w ]
set f2 [open $tfile.outbound w ]
while {[gets $fi line] >= 0} {

	set sTime [lindex $line 0]  

	set ipport [lindex $line 1]
	set s [split $ipport "."]
	set opport [lindex $line 3]
	set d [split $opport "."]
	set prefixc [format "%s.%s" [lindex $s 0] [lindex $s 1]]
	set prefixs [format "%s.%s" [lindex $d 0] [lindex $d 1]]


        #data packet from ISI
	if { $prefixc == $isiPrefix} {
                puts $f2 "$line"
	} else {
                puts $f1 "$line"
	}

}
close $f1 
close $f2 
close $fi 
