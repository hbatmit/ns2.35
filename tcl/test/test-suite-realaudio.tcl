# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Copyright (c) 1998 University of Southern California.
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-realaudio.tcl,v 1.4 2005/06/11 04:42:09 sfloyd Exp $

# To run all tests: test-all-realaudio
#
# To view a list of available test to run with this script:
# ns test-suite-realaudio.tcl

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP RTP TCP  ; # hdrs reqd for validation test

proc usage {}  {
        global argv0
	puts stderr "usage: ns $argv0 realaudio"
        exit 1
}


Class TestSuite

Class Test/realaudio -superclass TestSuite

TestSuite instproc init {} {
}

Test/realaudio instproc init {} {
##
global seed packetsize nn cnn plottime

set seed           1
set packetsize     245
set nn             3             ;# number of nodes
set cnn            [expr $nn -2 ]   ;# number of clients
set plottime 15000.0

}


Test/realaudio instproc run {} {
global seed packetsize nn cnn plottime

ns-random $seed

##user start time from a poisson distribution
set starttime(2) 0

#set tmp [new RandomVariable/Exponential] ;# Poisson process
#$tmp set avg_ 5.4674 ;# average arrival interval

#artificially syncronize flow start time
#every flow starts at multiple of 1.8s
set tmp [new RandomVariable/Empirical]
$tmp loadCDF userintercdf1

for {set i 3} {$i < $nn} {incr i} {
    set p [$tmp value]
    set i1 [expr $i - 1 ]
    set starttime($i) [expr $starttime($i1) + $p ]
}

##number of sequential flow per user
set rv0 [new RandomVariable/Empirical]
$rv0 loadCDF sflowcdf

##flow duration
set rv1 [new RandomVariable/Empirical]
$rv1 loadCDF flowdurcdf

for {set i 2} {$i < $nn} {incr i} {
  set q [$rv0 value]
  set sflow($i) [expr int($q) ]
#  puts "node $i has $sflow($i) flow "
  set p [$rv1 value]
  set dur($i) [expr $p * 60 ]
#  puts "node $i duration : $dur($i)"
}


for {set i 2} {$i < $nn} {incr i} {
    set flowstoptime($i) [expr $starttime($i) + $dur($i) ]
}


set ns [new Simulator]

for {set i 0} {$i < $nn} {incr i} {
set n($i) [$ns node]
}

set f [open temp.rands w]
$ns trace-all $f

$ns duplex-link $n(0) $n(1) 1.5Mb 10ms DropTail

for {set j 2} {$j < $nn} {incr j} {
    $ns duplex-link $n(0) $n($j) 10Mb 5ms DropTail
}

set rv2 [new RandomVariable/Empirical]
$rv2 loadCDF ontimecdf
set rv3 [new RandomVariable/Empirical]
$rv3 loadCDF fratecdf

for {set i 2} {$i < $nn} {incr i} {
  set s($i) [new Agent/UDP]
  $ns attach-agent $n(1) $s($i)
  set null($i) [new Agent/Null]
  $ns attach-agent $n($i) $null($i)
  $ns connect $s($i) $null($i)

  set realaudio($i) [new Application/Traffic/RealAudio]
  $realaudio($i) set packetSize_ $packetsize

  $realaudio($i) set burst_time_ 0.05ms
  $realaudio($i) set idle_time_ 1800ms


  set flow_rate  [$rv3 value]  
  set r [ format "%fk"  $flow_rate ]

#  puts "node $i flow rate $r"

  $realaudio($i) set rate_ $r
  $realaudio($i) attach-agent $s($i)
}



for {set i 2} {$i < $nn} {incr i} {

      $ns at $starttime($i) "$realaudio($i) start"
      $ns at $flowstoptime($i) "$realaudio($i) stop"

#      puts "node $i starttime $starttime($i)"
#      puts "node $i stoptime $flowstoptime($i)"
  

      ##schedule for next flow
      for {set h 2} {$h <= $sflow($i)} {incr h} {

          set starttime($i) [expr $flowstoptime($i) + 0.001 ]
          set p [$rv1 value]
          set dur($i) [expr $p * 60 ]
#         puts "node $i duration : $dur($i)"

	  set flowstoptime($i) [expr $starttime($i) + $dur($i) ]

          set realaudio($i) [new Application/Traffic/RealAudio]
          $realaudio($i) set packetSize_ $packetsize
          $realaudio($i) set burst_time_ 0.05ms
          $realaudio($i) set idle_time_ 1800ms

          set flow_rate  [$rv3 value]  
          set r [ format "%fk"  $flow_rate ]

#         puts "node $i flow rate $r"

          $realaudio($i) set rate_ $r
          $realaudio($i) attach-agent $s($i)
          
      }

}

$ns at $plottime "close $f"
$ns at $plottime "puts \"NS EXITING...\" ; $ns halt"

$ns run
}

TestSuite instproc finish {} {
	exec awk {
		{
			if (($1 == "+") && ($3 == 1) ) \
			     print $2, $10
		}
	} temp.rands > out.tr
        exit 0
}


proc runtest {arg} {
        global quiet
        set quiet 0

	set b [llength $arg]
	if {$b == 1} {
	        set test $arg
        } elseif {$b == 2} {
	        set test [lindex $arg 0]
            if {[lindex $arg 1] == "QUIET"} {
	        set quiet 1
            }
        } else {
	        usage
        }
	switch $test {
		realaudio {
                     set t [new Test/$test]
                }
                default {
	             stderr "Unknown test $test"
		     exit 1
                }
         }
	 $t run
}

global argv arg0 
runtest $argv


