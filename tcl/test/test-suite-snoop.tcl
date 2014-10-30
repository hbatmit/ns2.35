# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
Agent/TCP set tcpTick_ 0.1
# The default for tcpTick_ is being changed to reflect a changing reality.
Agent/TCP set rfc2988_ false
# The default for rfc2988_ is being changed to true.
#
# Copyright (c) 2001 University of Southern California.
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-snoop.tcl,v 1.8 2006/01/24 23:00:07 sallyfloyd Exp $

# This test suite is for validating the snoop protocol

remove-all-packet-headers       ; # removes all except common
add-packet-header Flags IP TCP Snoop Arp LL Mac ; # hdrs reqd for validation

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Agent/TCP set minrto_ 1
# default changed on 10/14/2004.
Agent/TCP set singledup_ 0
# The default is being changed to 1
Agent/TCP set useHeaders_ false
# The default is being changed to useHeaders_ true.

# global options
set opt(tr)	temp.rands
set opt(seed)	0
set opt(qsize)	100
set opt(ll)	LL
set opt(ifq)	Queue/DropTail
set opt(mac)	Mac/802_3
set opt(loss)	20

Class TestSuite
proc usage {} {
        global argv0
        puts stderr "usage: ns $argv0 <test> "
        puts "Valid Tests: simple"
        exit 1
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
        set t [new Test/$test]
        $t run
}


TestSuite instproc init {} {
  global opt tracefd
  $self instvar ns_
  set ns_ [new Simulator]
  set tracefd [open $opt(tr) w]
  $ns_ trace-all $tracefd
}

TestSuite instproc finish {} {
  $self instvar ns_ testName_
  global quiet

  $ns_ flush-trace

  puts "finishing.."
  exit 0
 }


# simple test
#
# Network topology:
#
#  A----B
#      _|_(lan)  
#       |
#       C----D
#
# A sends data to destination D
#
# B is snoop agent.  The link between C and D is lossy.
#
# Why the extra node C between B and D?  Because it's easier to model
# error on a point-to-point link than on a LAN in the current implementation.


Class Test/simple -superclass TestSuite

Test/simple instproc init {} {
  global opt
  $self instvar ns_ testName_
  set testName_ simple

  $self next

  
  set A [$ns_ node]
  set B [$ns_ node]
  set C [$ns_ node]
  set D [$ns_ node]

  $ns_ duplex-link $A $B 10Mb 5ms DropTail
  $ns_ duplex-link $C $D 10Mb 5ms DropTail

  set lan [$ns_ make-lan [list $C] 11Mb 2ms LL $opt(ifq) $opt(mac)]
  $lan addNode [list $B] 11Mb 2ms LL/LLSnoop $opt(ifq) $opt(mac)


  # make the link between C and D drop $opt(loss) percent of the packets
  set err [new ErrorModel]
  $err drop-target [new Agent/Null] 
  set rand [new RandomVariable/Uniform]
  $rand set min_ 0
  $rand set max_ 100
  $err ranvar $rand
  $err set rate_ $opt(loss)
  [$ns_ link $C $D] errormodule $err

  # set up TCP flow from A to D
  set tcp [new Agent/TCP]
  set sink [new Agent/TCPSink]
  $ns_ attach-agent $A $tcp
  $ns_ attach-agent $D $sink
  $ns_ connect $tcp $sink
  set ftp [new Application/FTP]
  $ftp attach-agent $tcp


  # set times to start/stop actions

  $ns_ at 0.1 "$ftp start"
  $ns_ at 1.9 "$ftp stop"
  $ns_ at 2.0 "$self finish"
}

Test/simple instproc run {} {
  $self instvar ns_
  puts "Starting Simulation..."
  $ns_ run
}


global argv arg0
runtest $argv
