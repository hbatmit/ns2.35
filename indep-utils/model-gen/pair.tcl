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
# An tcl script that find and pair SYN and SYN-ACK packets for each tcp 
# connection, used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066



if { $argc != 2} {
   	puts "usage: pair.tcl <tcpdump ASCII file> <prefix>"
   	exit
} else {
  	set arg [split $argv " " ]
  	set tfile [lindex $arg 0]
  	set prefix [lindex $arg 1]
}

set arg1 [split $prefix "." ]
set netPrefix [format "%s.%s" [lindex $arg1 0] [lindex $arg1 1]]


set prevClnt ""
set prevSvr ""
set fi [open $tfile r ]
while {[gets $fi line] >= 0} {

  	set client [lindex $line 0]
  	set server [lindex $line 1]

  	set time [lindex $line 2]

  	set c [split $client ".:" ]
  	set s [split $server ".:" ]

  	set Sequence [split [lindex $line 3] ":"]
  	set seq [lindex $Sequence 0]

  	set Client [format "%s.%s.%s.%s.%s" [lindex $c 0] [lindex $c 1] [lindex $c 2] [lindex $c 3] [lindex $c 4]]
  	set Server [format "%s.%s.%s.%s.%s" [lindex $s 0] [lindex $s 1] [lindex $s 2] [lindex $s 3] [lindex $s 4]]
  	set Client1 [format "%s.%s.%s.%s" [lindex $c 0] [lindex $c 1] [lindex $c 2] [lindex $c 3]]
  	set Server1 [format "%s.%s.%s.%s" [lindex $s 0] [lindex $s 1] [lindex $s 2] [lindex $s 3]]
  	set ClientD [format "%s.%s" [lindex $c 0] [lindex $c 1]]

  	set ack [lindex $line 4]
  	if { [lindex $line 5] != "win"} {
   		set seq1 [expr [lindex $line 5] - 1]

   		if { $Client == $prevClnt} {
    			if { $Server == $prevSvr} {
     				if { $ack == "ack"} {
      					if { $ClientD == $netPrefix} {
       						if { $seq1 == $prevSeq} {
        						puts "$Client1 $Server1 [expr $time - $prevTime]"
       						}
      					}
     				}
    			}
   		}
  	}
  	set prev $line
  	set prevClnt $Client
  	set prevSvr $Server
  	set prevSeq $seq
  	set prevTime $time 
}
close $fi 
