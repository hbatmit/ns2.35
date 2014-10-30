# -*-	Mode:tcl; tcl-indent-level:8; tab-width:8; indent-tabs-mode:t -*-
#
# Copyright (c) 2002 University of Southern California.
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
# $Header: /cvsroot/nsnam/ns-2/tcl/test/test-suite-tagged-trace.tcl,v 1.8 2006/01/30 21:27:52 mweigle Exp $

# FOR UPDATING GLOBAL DEFAULTS:
Agent/TCP set precisionReduce_ false ;   # default changed on 2006/1/24.
Agent/TCP set rtxcur_init_ 6.0 ;      # Default changed on 2006/01/21
Agent/TCP set updated_rttvar_ false ;  # Variable added on 2006/1/21
Mac/802_11 set bugFix_timer_ false;     # default changed 2006/1/30

# This test suite is for validating the tagged trace format


set opt(tr) "temp.rands"
set opt(chan)         Channel/WirelessChannel  ;# channel type
set opt(prop)         Propagation/TwoRayGround ;# radio-propagation model
set opt(ant)          Antenna/OmniAntenna      ;# Antenna type
set opt(ll)           LL                       ;# Link layer type
set opt(ifq)          Queue/DropTail/PriQueue  ;# Interface queue type
set opt(ifqlen)       50                       ;# max packet in ifq
set opt(netif)        Phy/WirelessPhy          ;# network interface type
set opt(mac)          Mac/802_11               ;# MAC type
set opt(rp)           DSDV                     ;# ad-hoc routing protocol 

Class TestSuite
proc usage {} {
        global argv0
        puts stderr "usage: ns $argv0 <test> "
        puts "Valid Tests: simple wireless Format-simple"
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
  $ns_ use-taggedtrace
  set tracefd [open $opt(tr) w]
  $ns_ trace-all $tracefd
}

TestSuite instproc finish {} {
  $self instvar ns_ testName_
  global quiet

  $ns_ flush-trace
  $ns_ halt
 }


# simple test
#
# Network topology:
#
#  A            E
#   \          /
#    ---C--D---
#   /          \
#  B            F
#
# A->E ftp
# B->F cbr
#
# C<->D link is lossy


Class Test/simple -superclass TestSuite

Test/simple instproc init {} {
  $self instvar ns_ testName_
  set testName_ simple

  $self next

  
  set A [$ns_ node]
  set B [$ns_ node]
  set C [$ns_ node]
  set D [$ns_ node]
  set E [$ns_ node]
  set F [$ns_ node]

  $ns_ duplex-link $A $C 10Mb 5ms DropTail
  $ns_ duplex-link $B $C 10Mb 5ms DropTail

  $ns_ duplex-link $C $D 2Mb 20ms DropTail

  $ns_ duplex-link $D $E 10Mb 5ms DropTail
  $ns_ duplex-link $D $F 10Mb 5ms DropTail



  # make the link between C and D drop 1% percent of the packets
  set err [new ErrorModel]
  $err drop-target [new Agent/Null] 
  set rand [new RandomVariable/Uniform]
  $rand set min_ 0
  $rand set max_ 100
  $err ranvar $rand
  $err set rate_ 0.01
  [$ns_ link $C $D] errormodule $err

  # set up TCP flow from A to E
  set tcp [new Agent/TCP]
  set sink [new Agent/TCPSink]
  $ns_ attach-agent $A $tcp
  $ns_ attach-agent $E $sink
  $ns_ connect $tcp $sink
  set ftp [new Application/FTP]
  $ftp attach-agent $tcp

  # set up CBR flow from B to F
  set udp [new Agent/UDP]
  set null [new Agent/Null]
  $ns_ attach-agent $B $udp
  $ns_ attach-agent $F $null
  $ns_ connect $udp $null
  set cbr [new Application/Traffic/CBR]
  $cbr set packetSize_ 500
  $cbr set interval_ 0.005
  $cbr attach-agent $udp

  # set times to start/stop actions

  $ns_ at 0.1 "$ftp start"
  $ns_ at 0.2 "$cbr start"
  $ns_ at 0.8 "$cbr stop"
  $ns_ at 1.2 "$cbr start"
  $ns_ at 1.8 "$cbr stop"
  $ns_ at 1.9 "$ftp stop"
  $ns_ at 2.0 "$self finish"
}

Test/simple instproc run {} {
  $self instvar ns_
  puts "Starting Simulation..."
  $ns_ run
}




# wireless test
#
# A->B ftp
# C->D cbr


Class Test/wireless -superclass TestSuite

Test/wireless instproc init {} {
  global opt
  $self instvar ns_ testName_
  set testName_ simple

  $self next

  set topo [new Topography]
  $topo load_flatgrid 100 100
  create-god 4
  $ns_ node-config -adhocRouting $opt(rp) \
                   -llType $opt(ll) \
                   -macType $opt(mac) \
                   -ifqType $opt(ifq) \
                   -ifqLen $opt(ifqlen) \
                   -antType $opt(ant) \
                   -propType $opt(prop) \
                   -phyType $opt(netif) \
                   -topoInstance $topo \
                   -channel [new $opt(chan)] \
                    -agentTrace ON \
                    -routerTrace ON \
                    -macTrace ON \
                    -movementTrace OFF

  set A [$ns_ node]
  $A random-motion 0
  $A set X_ 5.0
  $A set Y_ 2.0
  $A set Z_ 0.0
  set B [$ns_ node]
  $B random-motion 0
  $B set X_ 15.0
  $B set Y_ 15.0
  $B set Z_ 0.0
  set C [$ns_ node]
  $C random-motion 0
  $C set X_ 2.0
  $C set Y_ 8.0
  $C set Z_ 0.0
  set D [$ns_ node]
  $D random-motion 0
  $D set X_ 10.0
  $D set Y_ 1.0
  $D set Z_ 0.0


  # set up TCP flow from A to B
  set tcp [new Agent/TCP]
  set sink [new Agent/TCPSink]
  $ns_ attach-agent $A $tcp
  $ns_ attach-agent $B $sink
  $ns_ connect $tcp $sink
  set ftp [new Application/FTP]
  $ftp attach-agent $tcp

  # set up CBR flow from C to D
  set udp [new Agent/UDP]
  set null [new Agent/Null]
  $ns_ attach-agent $C $udp
  $ns_ attach-agent $D $null
  $ns_ connect $udp $null
  set cbr [new Application/Traffic/CBR]
  $cbr set packetSize_ 500
  $cbr set interval_ 0.005
  $cbr attach-agent $udp

  # set times to start/stop actions

  $ns_ at 0.1 "$ftp start"
  $ns_ at 0.2 "$cbr start"
  $ns_ at 0.8 "$cbr stop"
  $ns_ at 1.2 "$cbr start"
  $ns_ at 1.8 "$cbr stop"
  $ns_ at 1.9 "$ftp stop"
  $ns_ at 2.0 "$self finish"
}

Test/wireless instproc run {} {
  $self instvar ns_
  puts "Starting Simulation..."
  $ns_ run
}



# The following tests are for testing the file conversion utilities

Class Test/Format-simple -superclass Test/simple

Test/Format-simple instproc init {} {
  global PERL

  # check to make sure prereqs for file conversion are met

  set foo [catch {exec $PERL -I../../bin -MNS::TraceFileEvent -MNS::TraceFileReader -MNS::TraceFileWriter -e exit 2>/dev/null}]

  if [expr $foo != 0] then {
    puts "Required Perl module not found, Format-simple test skipped."
    exit 2
  }

  $self next
}

Test/Format-simple instproc run {} {
  global opt PERL

  # let parent class run ns
  $self next

  # now that ns is done, convert the output file
  exec $PERL -I../../bin ../../bin/ns2oldns.pl < $opt(tr) > $opt(tr).tmp
  exec cp $opt(tr).tmp $opt(tr)	
}





# main program .. runs the test specified by the command line arguments

global argv arg0
runtest $argv
