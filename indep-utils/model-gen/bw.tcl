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
# An tcl script that extracts timestamp and size of each packet
# from tcpdump trace, used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project 
# (http://www.isi.edu/saman/), administered by the Space and Naval 
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

if { $argc != 1} {
   puts "usage: bw.tcl <tcpdump file in ASCII format> "
   exit
} else {
  set arg [split $argv " " ]
  set tfile [lindex $arg 0]
}

set fi [open $tfile r ]
set fo [open $tfile.bw w ]
while {[gets $fi line] >= 0} {

	set startTime  [lindex $line 0]

        set seqno [lindex $line 5]
        if { $seqno !="ack"} {
        	set s [split $seqno ":()"]
     		set size [lindex $s 2]
     		set size [expr $size + 40]
     		puts $fo "$startTime $size"
    
	 } else {
	 	puts $fo "$startTime 40"
  	 }
}
close $fo 
close $fi 
