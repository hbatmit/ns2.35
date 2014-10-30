#!/usr/bin/tclsh


if { $argc != 1} {
   puts "usage: bw.tcl <tcpdump ASCII file> "
   exit
} else {
  set arg [split $argv " " ]
  set tfile [lindex $arg 0]
}

set fi [open $tfile r ]
while {[gets $fi line] >= 0} {

  set flag  [lindex $line 0]
  set startTime  [lindex $line 1]

  set size [lindex $line 5]

  set src [lindex $line 8]
  set dst [lindex $line 9]

  set s [split $src "."]
  set sip [lindex $s 0]

  set d [split $dst "."]
  set dip [lindex $d 0]

if { $flag == "+"} {
  if { $sip == 9 || $dip == 9} {
     if { $size == 1000 } {
        set size 1500
     }
     puts "$startTime $size"
  }
}
}
close $fi 
