# Copyright (c) 2010 Priscila Goncalves Doria
# Federal University of Campina Grande
# Brazil
# 
# This file is part of DCCP for NS-2.
# DCCP original source code is copyrighted by Nils-Erik Mattsson
# 
#    DCCP is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published
#    by the Free Software Foundation; either version 2 of the License,
#    or(at your option) any later version.
# 
#    DCCP is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with DCCP; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#    02110-1301 USA
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE

# A new simulador instance
set ns [new Simulator]

# Open a trace file
set tr [open dccp.tr w]
$ns trace-all $tr

# Open a NAM trace file
set nf [open dccp.nam w]
$ns namtrace-all $nf

# Creates 32 sending nodes
set s1 [$ns node]
set s2 [$ns node]
set s3 [$ns node]
set s4 [$ns node]
set s5 [$ns node]
set s6 [$ns node]
set s7 [$ns node]
set s8 [$ns node]
set s9 [$ns node]
set s10 [$ns node]
set s11 [$ns node]
set s12 [$ns node]
set s13 [$ns node]
set s14 [$ns node]
set s15 [$ns node]
set s16 [$ns node]
set s17 [$ns node]
set s18 [$ns node]
set s19 [$ns node]
set s20 [$ns node]
set s21 [$ns node]
set s22 [$ns node]
set s23 [$ns node]
set s24 [$ns node]
set s25 [$ns node]
set s26 [$ns node]
set s27 [$ns node]
set s28 [$ns node]
set s29 [$ns node]
set s30 [$ns node]
set s31 [$ns node]
set s32 [$ns node]

# Creates 4 routers
set a [$ns node]
set b [$ns node]
set c [$ns node]
set d [$ns node]

# Creates 32 receiving nodes
set r1 [$ns node]
set r2 [$ns node]
set r3 [$ns node]
set r4 [$ns node]
set r5 [$ns node]
set r6 [$ns node]
set r7 [$ns node]
set r8 [$ns node]
set r9 [$ns node]
set r10 [$ns node]
set r11 [$ns node]
set r12 [$ns node]
set r13 [$ns node]
set r14 [$ns node]
set r15 [$ns node]
set r16 [$ns node]
set r17 [$ns node]
set r18 [$ns node]
set r19 [$ns node]
set r20 [$ns node]
set r21 [$ns node]
set r22 [$ns node]
set r23 [$ns node]
set r24 [$ns node]
set r25 [$ns node]
set r26 [$ns node]
set r27 [$ns node]
set r28 [$ns node]
set r29 [$ns node]
set r30 [$ns node]
set r31 [$ns node]
set r32 [$ns node]

# Seed the default RNG
global defaultRNG
$defaultRNG seed 0

# Random number generator
set rng [new RNG] 
$rng seed 0

# Connect all nodes. Make them oriented for NAM
$ns duplex-link $a $s1  10Mb 1ms  DropTail
$ns duplex-link $a $s2  10Mb 1ms  DropTail
$ns duplex-link $a $s3  10Mb 1ms  DropTail
$ns duplex-link $a $s4  10Mb 1ms  DropTail
$ns duplex-link $a $s5  10Mb 1ms  DropTail
$ns duplex-link $a $s6  10Mb 1ms  DropTail
$ns duplex-link $a $s7  10Mb 1ms  DropTail
$ns duplex-link $a $s8  10Mb 1ms  DropTail
$ns duplex-link $a $r17 10Mb 1ms  DropTail
$ns duplex-link $a $r18 10Mb 1ms  DropTail
$ns duplex-link $a $r19 10Mb 1ms  DropTail
$ns duplex-link $a $r20 10Mb 1ms  DropTail
$ns duplex-link $a $r21 10Mb 1ms  DropTail
$ns duplex-link $a $r22 10Mb 1ms  DropTail
$ns duplex-link $a $r23 10Mb 1ms  DropTail
$ns duplex-link $a $r24 10Mb 1ms  DropTail

$ns duplex-link $a $b   1Mb  20ms DropTail 
$ns duplex-link $b $c   1Mb  20ms DropTail
$ns duplex-link $c $d   1Mb  20ms DropTail
$ns duplex-link $d $a   1Mb  20ms DropTail

$ns duplex-link $b $s9  10Mb 1ms  DropTail
$ns duplex-link $b $s10 10Mb 1ms  DropTail
$ns duplex-link $b $s11 10Mb 1ms  DropTail
$ns duplex-link $b $s12 10Mb 1ms  DropTail
$ns duplex-link $b $s13 10Mb 1ms  DropTail
$ns duplex-link $b $s14 10Mb 1ms  DropTail
$ns duplex-link $b $s15 10Mb 1ms  DropTail
$ns duplex-link $b $s16 10Mb 1ms  DropTail
$ns duplex-link $b $r25 10Mb 1ms  DropTail
$ns duplex-link $b $r26 10Mb 1ms  DropTail
$ns duplex-link $b $r27 10Mb 1ms  DropTail
$ns duplex-link $b $r28 10Mb 1ms  DropTail
$ns duplex-link $b $r29 10Mb 1ms  DropTail
$ns duplex-link $b $r30 10Mb 1ms  DropTail
$ns duplex-link $b $r31 10Mb 1ms  DropTail
$ns duplex-link $b $r32 10Mb 1ms  DropTail

$ns duplex-link $c $s17 10Mb 1ms  DropTail
$ns duplex-link $c $s18 10Mb 1ms  DropTail
$ns duplex-link $c $s19 10Mb 1ms  DropTail
$ns duplex-link $c $s20 10Mb 1ms  DropTail
$ns duplex-link $c $s21 10Mb 1ms  DropTail
$ns duplex-link $c $s22 10Mb 1ms  DropTail
$ns duplex-link $c $s23 10Mb 1ms  DropTail
$ns duplex-link $c $s24 10Mb 1ms  DropTail
$ns duplex-link $c $r1  10Mb 1ms  DropTail
$ns duplex-link $c $r2  10Mb 1ms  DropTail
$ns duplex-link $c $r3  10Mb 1ms  DropTail
$ns duplex-link $c $r4  10Mb 1ms  DropTail
$ns duplex-link $c $r5  10Mb 1ms  DropTail
$ns duplex-link $c $r6  10Mb 1ms  DropTail
$ns duplex-link $c $r7  10Mb 1ms  DropTail
$ns duplex-link $c $r8  10Mb 1ms  DropTail

$ns duplex-link $d $s25 10Mb 1ms  DropTail
$ns duplex-link $d $s26 10Mb 1ms  DropTail
$ns duplex-link $d $s27 10Mb 1ms  DropTail
$ns duplex-link $d $s28 10Mb 1ms  DropTail
$ns duplex-link $d $s29 10Mb 1ms  DropTail
$ns duplex-link $d $s30 10Mb 1ms  DropTail
$ns duplex-link $d $s31 10Mb 1ms  DropTail
$ns duplex-link $d $s32 10Mb 1ms  DropTail
$ns duplex-link $d $r9  10Mb 1ms  DropTail
$ns duplex-link $d $r10 10Mb 1ms  DropTail
$ns duplex-link $d $r11 10Mb 1ms  DropTail
$ns duplex-link $d $r12 10Mb 1ms  DropTail
$ns duplex-link $d $r13 10Mb 1ms  DropTail
$ns duplex-link $d $r14 10Mb 1ms  DropTail
$ns duplex-link $d $r15 10Mb 1ms  DropTail
$ns duplex-link $d $r16 10Mb 1ms  DropTail

# Set the link queue size (100 packets)
$ns queue-limit $a $b 100
$ns queue-limit $b $c 100
$ns queue-limit $c $d 100
$ns queue-limit $d $a 100


# DCCP agents and respective sinks.
# 16 TCPLike DCCP agents (CCID2), 16 TRFC DCCP agents(CCID3).

# DCCP agent #1
set dccp1 [new Agent/DCCP/TCPlike]
set dccpsink1 [new Agent/DCCP/TCPlike]
$ns attach-agent $s1 $dccp1
$ns attach-agent $r1 $dccpsink1
$ns connect $dccp1 $dccpsink1
$dccp1 set fid_ 1

# DCCP agent #2
set dccp2 [new Agent/DCCP/TCPlike]
set dccpsink2 [new Agent/DCCP/TCPlike]
$ns attach-agent $s2 $dccp2
$ns attach-agent $r2 $dccpsink2
$ns connect $dccp2 $dccpsink2
$dccp2 set fid_ 2

# DCCP agent #3
set dccp3 [new Agent/DCCP/TCPlike]
set dccpsink3 [new Agent/DCCP/TCPlike]
$ns attach-agent $s3 $dccp3
$ns attach-agent $r3 $dccpsink3
$ns connect $dccp3 $dccpsink3
$dccp3 set fid_ 3

# DCCP agent #4
set dccp4 [new Agent/DCCP/TCPlike]
set dccpsink4 [new Agent/DCCP/TCPlike]
$ns attach-agent $s4 $dccp4
$ns attach-agent $r4 $dccpsink4
$ns connect $dccp4 $dccpsink4
$dccp4 set fid_ 4

# DCCP agent #5
set dccp5 [new Agent/DCCP/TCPlike]
set dccpsink5 [new Agent/DCCP/TCPlike]
$ns attach-agent $s5 $dccp5
$ns attach-agent $r5 $dccpsink5
$ns connect $dccp5 $dccpsink5
$dccp5 set fid_ 5

# DCCP agent #6
set dccp6 [new Agent/DCCP/TCPlike]
set dccpsink6 [new Agent/DCCP/TCPlike]
$ns attach-agent $s6 $dccp6
$ns attach-agent $r6 $dccpsink6
$ns connect $dccp6 $dccpsink6
$dccp6 set fid_ 6

# DCCP agent #7
set dccp7 [new Agent/DCCP/TCPlike]
set dccpsink7 [new Agent/DCCP/TCPlike]
$ns attach-agent $s7 $dccp7
$ns attach-agent $r7 $dccpsink7
$ns connect $dccp7 $dccpsink7
$dccp7 set fid_ 7

# DCCP agent #8
set dccp8 [new Agent/DCCP/TCPlike]
set dccpsink8 [new Agent/DCCP/TCPlike]
$ns attach-agent $s8 $dccp8
$ns attach-agent $r8 $dccpsink8
$ns connect $dccp8 $dccpsink8
$dccp8 set fid_ 8

# DCCP agent #9
set dccp9 [new Agent/DCCP/TCPlike]
set dccpsink9 [new Agent/DCCP/TCPlike]
$ns attach-agent $s9 $dccp9
$ns attach-agent $r9 $dccpsink9
$ns connect $dccp9 $dccpsink9
$dccp9 set fid_ 9

# DCCP agent #10
set dccp10 [new Agent/DCCP/TCPlike]
set dccpsink10 [new Agent/DCCP/TCPlike]
$ns attach-agent $s10 $dccp10
$ns attach-agent $r10 $dccpsink10
$ns connect $dccp10 $dccpsink10
$dccp10 set fid_ 10

# DCCP agent #11
set dccp11 [new Agent/DCCP/TCPlike]
set dccpsink11 [new Agent/DCCP/TCPlike]
$ns attach-agent $s11 $dccp11
$ns attach-agent $r11 $dccpsink11
$ns connect $dccp11 $dccpsink11
$dccp11 set fid_ 11

# DCCP agent #12
set dccp12 [new Agent/DCCP/TCPlike]
set dccpsink12 [new Agent/DCCP/TCPlike]
$ns attach-agent $s12 $dccp12
$ns attach-agent $r12 $dccpsink12
$ns connect $dccp12 $dccpsink12
$dccp12 set fid_ 12

# DCCP agent #13
set dccp13 [new Agent/DCCP/TCPlike]
set dccpsink13 [new Agent/DCCP/TCPlike]
$ns attach-agent $s13 $dccp13
$ns attach-agent $r13 $dccpsink13
$ns connect $dccp13 $dccpsink13
$dccp13 set fid_ 13

# DCCP agent #14
set dccp14 [new Agent/DCCP/TCPlike]
set dccpsink14 [new Agent/DCCP/TCPlike]
$ns attach-agent $s14 $dccp14
$ns attach-agent $r14 $dccpsink14
$ns connect $dccp14 $dccpsink14
$dccp14 set fid_ 14

# DCCP agent #15
set dccp15 [new Agent/DCCP/TCPlike]
set dccpsink15 [new Agent/DCCP/TCPlike]
$ns attach-agent $s15 $dccp15
$ns attach-agent $r15 $dccpsink15
$ns connect $dccp15 $dccpsink15
$dccp15 set fid_ 15

# DCCP agent #16
set dccp16 [new Agent/DCCP/TCPlike]
set dccpsink16 [new Agent/DCCP/TCPlike]
$ns attach-agent $s16 $dccp16
$ns attach-agent $r16 $dccpsink16
$ns connect $dccp16 $dccpsink16
$dccp16 set fid_ 16

# DCCP agent #17
set dccp17 [new Agent/DCCP/TFRC]
set dccpsink17 [new Agent/DCCP/TFRC]
$ns attach-agent $s17 $dccp17
$ns attach-agent $r17 $dccpsink17
$ns connect $dccp17 $dccpsink17
$dccp17 set fid_ 17

# DCCP agent #18
set dccp18 [new Agent/DCCP/TFRC]
set dccpsink18 [new Agent/DCCP/TFRC]
$ns attach-agent $s18 $dccp18
$ns attach-agent $r18 $dccpsink18
$ns connect $dccp18 $dccpsink18
$dccp18 set fid_ 18

# DCCP agent #19
set dccp19 [new Agent/DCCP/TFRC]
set dccpsink19 [new Agent/DCCP/TFRC]
$ns attach-agent $s19 $dccp19
$ns attach-agent $r19 $dccpsink19
$ns connect $dccp19 $dccpsink19
$dccp19 set fid_ 19

# DCCP agent #20
set dccp20 [new Agent/DCCP/TFRC]
set dccpsink20 [new Agent/DCCP/TFRC]
$ns attach-agent $s20 $dccp20
$ns attach-agent $r20 $dccpsink20
$ns connect $dccp20 $dccpsink20
$dccp20 set fid_ 20

# DCCP agent #21
set dccp21 [new Agent/DCCP/TFRC]
set dccpsink21 [new Agent/DCCP/TFRC]
$ns attach-agent $s21 $dccp21
$ns attach-agent $r21 $dccpsink21
$ns connect $dccp21 $dccpsink21
$dccp21 set fid_ 21

# DCCP agent #22
set dccp22 [new Agent/DCCP/TFRC]
set dccpsink22 [new Agent/DCCP/TFRC]
$ns attach-agent $s22 $dccp22
$ns attach-agent $r22 $dccpsink22
$ns connect $dccp22 $dccpsink22
$dccp22 set fid_ 22

# DCCP agent #23
set dccp23 [new Agent/DCCP/TFRC]
set dccpsink23 [new Agent/DCCP/TFRC]
$ns attach-agent $s23 $dccp23
$ns attach-agent $r23 $dccpsink23
$ns connect $dccp23 $dccpsink23
$dccp23 set fid_ 23

# DCCP agent #24
set dccp24 [new Agent/DCCP/TFRC]
set dccpsink24 [new Agent/DCCP/TFRC]
$ns attach-agent $s24 $dccp24
$ns attach-agent $r24 $dccpsink24
$ns connect $dccp24 $dccpsink24
$dccp24 set fid_ 24

# DCCP agent #25
set dccp25 [new Agent/DCCP/TFRC]
set dccpsink25 [new Agent/DCCP/TFRC]
$ns attach-agent $s25 $dccp25
$ns attach-agent $r25 $dccpsink25
$ns connect $dccp25 $dccpsink25
$dccp25 set fid_ 25

# DCCP agent #26
set dccp26 [new Agent/DCCP/TFRC]
set dccpsink26 [new Agent/DCCP/TFRC]
$ns attach-agent $s26 $dccp26
$ns attach-agent $r26 $dccpsink26
$ns connect $dccp26 $dccpsink26
$dccp26 set fid_ 26

# DCCP agent #27
set dccp27 [new Agent/DCCP/TFRC]
set dccpsink27 [new Agent/DCCP/TFRC]
$ns attach-agent $s27 $dccp27
$ns attach-agent $r27 $dccpsink27
$ns connect $dccp27 $dccpsink27
$dccp27 set fid_ 27

# DCCP agent #28
set dccp28 [new Agent/DCCP/TFRC]
set dccpsink28 [new Agent/DCCP/TFRC]
$ns attach-agent $s28 $dccp28
$ns attach-agent $r28 $dccpsink28
$ns connect $dccp28 $dccpsink28
$dccp28 set fid_ 28

# DCCP agent #29
set dccp29 [new Agent/DCCP/TFRC]
set dccpsink29 [new Agent/DCCP/TFRC]
$ns attach-agent $s29 $dccp29
$ns attach-agent $r29 $dccpsink29
$ns connect $dccp29 $dccpsink29
$dccp29 set fid_ 29

# DCCP agent #30
set dccp30 [new Agent/DCCP/TFRC]
set dccpsink30 [new Agent/DCCP/TFRC]
$ns attach-agent $s30 $dccp30
$ns attach-agent $r30 $dccpsink30
$ns connect $dccp30 $dccpsink30
$dccp30 set fid_ 30

# DCCP agent #31
set dccp31 [new Agent/DCCP/TFRC]
set dccpsink31 [new Agent/DCCP/TFRC]
$ns attach-agent $s31 $dccp31
$ns attach-agent $r31 $dccpsink31
$ns connect $dccp31 $dccpsink31
$dccp31 set fid_ 31

# DCCP agent #32
set dccp32 [new Agent/DCCP/TFRC]
set dccpsink32 [new Agent/DCCP/TFRC]
$ns attach-agent $s32 $dccp32
$ns attach-agent $r32 $dccpsink32
$ns connect $dccp32 $dccpsink32
$dccp32 set fid_ 32

# Window size (only for CCID2 agents)

$dccp1 set window_ 7000
$dccp2 set window_ 7000
$dccp3 set window_ 7000
$dccp4 set window_ 7000
$dccp5 set window_ 7000
$dccp6 set window_ 7000
$dccp7 set window_ 7000
$dccp8 set window_ 7000
$dccp9 set window_ 7000
$dccp10 set window_ 7000
$dccp11 set window_ 7000
$dccp12 set window_ 7000
$dccp13 set window_ 7000
$dccp14 set window_ 7000
$dccp15 set window_ 7000
$dccp16 set window_ 7000

# CBR #1 (DCCP1)
set cbr1 [new Application/Traffic/CBR]
$cbr1 attach-agent $dccp1
$cbr1 set packetSize_ 160
$cbr1 set rate_ 80Kb
$cbr1 set random_ rng 

# CBR #2 (DCCP2)
set cbr2 [new Application/Traffic/CBR]
$cbr2 attach-agent $dccp2
$cbr2 set packetSize_ 160
$cbr2 set rate_ 80Kb
$cbr2 set random_ rng 

# CBR #3 (DCCP3)
set cbr3 [new Application/Traffic/CBR]
$cbr3 attach-agent $dccp3
$cbr3 set packetSize_ 160
$cbr3 set rate_ 80Kb
$cbr3 set random_ rng 

# CBR #4 (DCCP4)
set cbr4 [new Application/Traffic/CBR]
$cbr4 attach-agent $dccp4
$cbr4 set packetSize_ 160
$cbr4 set rate_ 80Kb
$cbr4 set random_ rng 

# CBR #5 (DCCP5)
set cbr5 [new Application/Traffic/CBR]
$cbr5 attach-agent $dccp5
$cbr5 set packetSize_ 160
$cbr5 set rate_ 80Kb
$cbr5 set random_ rng 

# CBR #6 (DCCP6)
set cbr6 [new Application/Traffic/CBR]
$cbr6 attach-agent $dccp6
$cbr6 set packetSize_ 160
$cbr6 set rate_ 80Kb
$cbr6 set random_ rng 

# CBR #7 (DCCP7)
set cbr7 [new Application/Traffic/CBR]
$cbr7 attach-agent $dccp7
$cbr7 set packetSize_ 160
$cbr7 set rate_ 80Kb
$cbr7 set random_ rng 

# CBR #8 (DCCP8)
set cbr8 [new Application/Traffic/CBR]
$cbr8 attach-agent $dccp8
$cbr8 set packetSize_ 160
$cbr8 set rate_ 80Kb
$cbr8 set random_ rng 

# CBR #9 (DCCP9)
set cbr9 [new Application/Traffic/CBR]
$cbr9 attach-agent $dccp9
$cbr9 set packetSize_ 160
$cbr9 set rate_ 80Kb
$cbr9 set random_ rng 

# CBR #10 (DCCP10)
set cbr10 [new Application/Traffic/CBR]
$cbr10 attach-agent $dccp10
$cbr10 set packetSize_ 160
$cbr10 set rate_ 80Kb
$cbr10 set random_ rng 

# CBR #11 (DCCP11)
set cbr11 [new Application/Traffic/CBR]
$cbr11 attach-agent $dccp11
$cbr11 set packetSize_ 160
$cbr11 set rate_ 80Kb
$cbr11 set random_ rng 

# CBR #12 (DCCP12)
set cbr12 [new Application/Traffic/CBR]
$cbr12 attach-agent $dccp12
$cbr12 set packetSize_ 160
$cbr12 set rate_ 80Kb
$cbr12 set random_ rng 

# CBR #13 (DCCP13)
set cbr13 [new Application/Traffic/CBR]
$cbr13 attach-agent $dccp13
$cbr13 set packetSize_ 160
$cbr13 set rate_ 80Kb
$cbr13 set random_ rng 

# CBR #14 (DCCP14)
set cbr14 [new Application/Traffic/CBR]
$cbr14 attach-agent $dccp14
$cbr14 set packetSize_ 160
$cbr14 set rate_ 80Kb
$cbr14 set random_ rng 

# CBR #15 (DCCP15)
set cbr15 [new Application/Traffic/CBR]
$cbr15 attach-agent $dccp15
$cbr15 set packetSize_ 160
$cbr15 set rate_ 80Kb
$cbr15 set random_ rng 

# CBR #16 (DCCP16)
set cbr16 [new Application/Traffic/CBR]
$cbr16 attach-agent $dccp16
$cbr16 set packetSize_ 160
$cbr16 set rate_ 80Kb
$cbr16 set random_ rng 

# CBR #17 (DCCP17)
set cbr17 [new Application/Traffic/CBR]
$cbr17 attach-agent $dccp17
$cbr17 set packetSize_ 160
$cbr17 set rate_ 80Kb
$cbr17 set random_ rng 

# CBR #18 (DCCP18)
set cbr18 [new Application/Traffic/CBR]
$cbr18 attach-agent $dccp18
$cbr18 set packetSize_ 160
$cbr18 set rate_ 80Kb
$cbr18 set random_ rng 

# CBR #19 (DCCP19)
set cbr19 [new Application/Traffic/CBR]
$cbr19 attach-agent $dccp19
$cbr19 set packetSize_ 160
$cbr19 set rate_ 80Kb
$cbr19 set random_ rng 

# CBR #20 (DCCP20)
set cbr20 [new Application/Traffic/CBR]
$cbr20 attach-agent $dccp20
$cbr20 set packetSize_ 160
$cbr20 set rate_ 80Kb
$cbr20 set random_ rng 

# CBR #21 (DCCP21)
set cbr21 [new Application/Traffic/CBR]
$cbr21 attach-agent $dccp21
$cbr21 set packetSize_ 160
$cbr21 set rate_ 80Kb
$cbr21 set random_ rng 

# CBR #22 (DCCP22)
set cbr22 [new Application/Traffic/CBR]
$cbr22 attach-agent $dccp22
$cbr22 set packetSize_ 160
$cbr22 set rate_ 80Kb
$cbr22 set random_ rng 

# CBR #23 (DCCP23)
set cbr23 [new Application/Traffic/CBR]
$cbr23 attach-agent $dccp23
$cbr23 set packetSize_ 160
$cbr23 set rate_ 80Kb
$cbr23 set random_ rng 

# CBR #24 (DCCP24)
set cbr24 [new Application/Traffic/CBR]
$cbr24 attach-agent $dccp24
$cbr24 set packetSize_ 160
$cbr24 set rate_ 80Kb
$cbr24 set random_ rng 

# CBR #25 (DCCP25)
set cbr25 [new Application/Traffic/CBR]
$cbr25 attach-agent $dccp25
$cbr25 set packetSize_ 160
$cbr25 set rate_ 80Kb

# CBR #26 (DCCP26)
set cbr26 [new Application/Traffic/CBR]
$cbr26 attach-agent $dccp26
$cbr26 set packetSize_ 160
$cbr26 set rate_ 80Kb
$cbr26 set random_ rng 

# CBR #27 (DCCP27)
set cbr27 [new Application/Traffic/CBR]
$cbr27 attach-agent $dccp27
$cbr27 set packetSize_ 160
$cbr27 set rate_ 80Kb
$cbr27 set random_ rng 

# CBR #28 (DCCP28)
set cbr28 [new Application/Traffic/CBR]
$cbr28 attach-agent $dccp28
$cbr28 set packetSize_ 160
$cbr28 set rate_ 80Kb
$cbr28 set random_ rng 

# CBR #29 (DCCP29)
set cbr29 [new Application/Traffic/CBR]
$cbr29 attach-agent $dccp29
$cbr29 set packetSize_ 160
$cbr29 set rate_ 80Kb
$cbr29 set random_ rng 

# CBR #30 (DCCP30)
set cbr30 [new Application/Traffic/CBR]
$cbr30 attach-agent $dccp30
$cbr30 set packetSize_ 160
$cbr30 set rate_ 80Kb
$cbr30 set random_ rng 

# CBR #31 (DCCP31)
set cbr31 [new Application/Traffic/CBR]
$cbr31 attach-agent $dccp31
$cbr31 set packetSize_ 160
$cbr31 set rate_ 80Kb
$cbr31 set random_ rng 

# CBR #32 (DCCP32)
set cbr32 [new Application/Traffic/CBR]
$cbr32 attach-agent $dccp32
$cbr32 set packetSize_ 160
$cbr32 set rate_ 80Kb
$cbr32 set random_ rng 

# Event scheduling
# Here you can enable or disable any sender/receiver

$ns at 0.0 "$dccpsink1 listen"
$ns at 0.0 "$dccpsink2 listen"
$ns at 0.0 "$dccpsink3 listen"
$ns at 0.0 "$dccpsink4 listen"
#$ns at 0.0 "$dccpsink5 listen"
#$ns at 0.0 "$dccpsink6 listen"
#$ns at 0.0 "$dccpsink7 listen"
#$ns at 0.0 "$dccpsink8 listen"
$ns at 0.0 "$dccpsink9 listen"
$ns at 0.0 "$dccpsink10 listen"
$ns at 0.0 "$dccpsink11 listen"
$ns at 0.0 "$dccpsink12 listen"
#$ns at 0.0 "$dccpsink13 listen"
#$ns at 0.0 "$dccpsink14 listen"
#$ns at 0.0 "$dccpsink15 listen"
#$ns at 0.0 "$dccpsink16 listen"
$ns at 0.0 "$dccpsink17 listen"
$ns at 0.0 "$dccpsink18 listen"
$ns at 0.0 "$dccpsink19 listen"
$ns at 0.0 "$dccpsink20 listen"
#$ns at 0.0 "$dccpsink21 listen"
#$ns at 0.0 "$dccpsink22 listen"
#$ns at 0.0 "$dccpsink23 listen"
#$ns at 0.0 "$dccpsink24 listen"
$ns at 0.0 "$dccpsink25 listen"
$ns at 0.0 "$dccpsink26 listen"
$ns at 0.0 "$dccpsink27 listen"
$ns at 0.0 "$dccpsink28 listen"
#$ns at 0.0 "$dccpsink29 listen"
#$ns at 0.0 "$dccpsink30 listen"
#$ns at 0.0 "$dccpsink31 listen"
#$ns at 0.0 "$dccpsink32 listen"

$ns at 0.0 "$cbr1 start"
$ns at 0.0 "$cbr2 start"
$ns at 0.0 "$cbr3 start"
$ns at 0.0 "$cbr4 start"
#$ns at 0.0 "$cbr5 start"
#$ns at 0.0 "$cbr6 start"
#$ns at 0.0 "$cbr7 start"
#$ns at 0.0 "$cbr8 start"
$ns at 0.0 "$cbr9 start"
$ns at 0.0 "$cbr10 start"
$ns at 0.0 "$cbr11 start"
$ns at 0.0 "$cbr12 start"
#$ns at 0.0 "$cbr13 start"
#$ns at 0.0 "$cbr14 start"
#$ns at 0.0 "$cbr15 start"
#$ns at 0.0 "$cbr16 start"
$ns at 0.0 "$cbr17 start"
$ns at 0.0 "$cbr18 start"
$ns at 0.0 "$cbr19 start"
$ns at 0.0 "$cbr20 start"
#$ns at 0.0 "$cbr21 start"
#$ns at 0.0 "$cbr22 start"
#$ns at 0.0 "$cbr23 start"
#$ns at 0.0 "$cbr24 start"
$ns at 0.0 "$cbr25 start"
$ns at 0.0 "$cbr26 start"
$ns at 0.0 "$cbr27 start"
$ns at 0.0 "$cbr28 start"
#$ns at 0.0 "$cbr29 start"
#$ns at 0.0 "$cbr30 start"
#$ns at 0.0 "$cbr31 start"
#$ns at 0.0 "$cbr32 start"

# Finish the simulation after 900s.
$ns at 900.0 "finish"

# Finish routine
proc finish {} {
     global ns tr nf
     $ns flush-trace
     close $tr
     close $nf
     puts "End of the Simulation. Issues? Contact me:  
plgdoria@gmail.com"
     exit 0
}

# Simulation Description
puts "Starting DCCP CCID2 x DCCP CCID3 simulation"

# Runs the simulation
$ns run
