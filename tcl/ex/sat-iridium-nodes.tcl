#
# Copyright (c) 1999 Regents of the University of California.
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
#       This product includes software developed by the MASH Research
#       Group at the University of California Berkeley.
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
# Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
#

if {![info exists ns]} {
	puts "Error:  sat-iridium-nodes.tcl is a supporting script for the "
	puts "        sat-iridium.tcl script-- run `sat-iridium.tcl' instead"
	exit
}

set plane 1
set n0 [$ns node]; $n0 set-position $alt $inc 0 0 $plane
set n1 [$ns node]; $n1 set-position $alt $inc 0 32.73 $plane 
set n2 [$ns node]; $n2 set-position $alt $inc 0 65.45 $plane 
set n3 [$ns node]; $n3 set-position $alt $inc 0 98.18 $plane 
set n4 [$ns node]; $n4 set-position $alt $inc 0 130.91 $plane 
set n5 [$ns node]; $n5 set-position $alt $inc 0 163.64 $plane 
set n6 [$ns node]; $n6 set-position $alt $inc 0 196.36 $plane 
set n7 [$ns node]; $n7 set-position $alt $inc 0 229.09 $plane 
set n8 [$ns node]; $n8 set-position $alt $inc 0 261.82 $plane 
set n9 [$ns node]; $n9 set-position $alt $inc 0 294.55 $plane 
set n10 [$ns node]; $n10 set-position $alt $inc 0 327.27 $plane 

incr plane  
set n15 [$ns node]; $n15 set-position $alt $inc 31.6 16.36 $plane 
set n16 [$ns node]; $n16 set-position $alt $inc 31.6 49.09 $plane 
set n17 [$ns node]; $n17 set-position $alt $inc 31.6 81.82 $plane 
set n18 [$ns node]; $n18 set-position $alt $inc 31.6 114.55 $plane 
set n19 [$ns node]; $n19 set-position $alt $inc 31.6 147.27 $plane 
set n20 [$ns node]; $n20 set-position $alt $inc 31.6 180 $plane 
set n21 [$ns node]; $n21 set-position $alt $inc 31.6 212.73 $plane 
set n22 [$ns node]; $n22 set-position $alt $inc 31.6 245.45 $plane 
set n23 [$ns node]; $n23 set-position $alt $inc 31.6 278.18 $plane 
set n24 [$ns node]; $n24 set-position $alt $inc 31.6 310.91 $plane 
set n25 [$ns node]; $n25 set-position $alt $inc 31.6 343.64 $plane 

incr plane 
set n30 [$ns node]; $n30 set-position $alt $inc 63.2 0 $plane 
set n31 [$ns node]; $n31 set-position $alt $inc 63.2 32.73 $plane 
set n32 [$ns node]; $n32 set-position $alt $inc 63.2 65.45 $plane 
set n33 [$ns node]; $n33 set-position $alt $inc 63.2 98.18 $plane 
set n34 [$ns node]; $n34 set-position $alt $inc 63.2 130.91 $plane 
set n35 [$ns node]; $n35 set-position $alt $inc 63.2 163.64 $plane 
set n36 [$ns node]; $n36 set-position $alt $inc 63.2 196.36 $plane 
set n37 [$ns node]; $n37 set-position $alt $inc 63.2 229.09 $plane 
set n38 [$ns node]; $n38 set-position $alt $inc 63.2 261.82 $plane 
set n39 [$ns node]; $n39 set-position $alt $inc 63.2 294.55 $plane 
set n40 [$ns node]; $n40 set-position $alt $inc 63.2 327.27 $plane 

incr plane 
set n45 [$ns node]; $n45 set-position $alt $inc 94.8 16.36 $plane 
set n46 [$ns node]; $n46 set-position $alt $inc 94.8 49.09 $plane 
set n47 [$ns node]; $n47 set-position $alt $inc 94.8 81.82 $plane 
set n48 [$ns node]; $n48 set-position $alt $inc 94.8 114.55 $plane 
set n49 [$ns node]; $n49 set-position $alt $inc 94.8 147.27 $plane 
set n50 [$ns node]; $n50 set-position $alt $inc 94.8 180 $plane 
set n51 [$ns node]; $n51 set-position $alt $inc 94.8 212.73 $plane 
set n52 [$ns node]; $n52 set-position $alt $inc 94.8 245.45 $plane 
set n53 [$ns node]; $n53 set-position $alt $inc 94.8 278.18 $plane 
set n54 [$ns node]; $n54 set-position $alt $inc 94.8 310.91 $plane 
set n55 [$ns node]; $n55 set-position $alt $inc 94.8 343.64 $plane 

incr plane 
set n60 [$ns node]; $n60 set-position $alt $inc 126.4 0 $plane 
set n61 [$ns node]; $n61 set-position $alt $inc 126.4 32.73 $plane 
set n62 [$ns node]; $n62 set-position $alt $inc 126.4 65.45 $plane 
set n63 [$ns node]; $n63 set-position $alt $inc 126.4 98.18 $plane 
set n64 [$ns node]; $n64 set-position $alt $inc 126.4 130.91 $plane 
set n65 [$ns node]; $n65 set-position $alt $inc 126.4 163.64 $plane 
set n66 [$ns node]; $n66 set-position $alt $inc 126.4 196.36 $plane 
set n67 [$ns node]; $n67 set-position $alt $inc 126.4 229.09 $plane 
set n68 [$ns node]; $n68 set-position $alt $inc 126.4 261.82 $plane 
set n69 [$ns node]; $n69 set-position $alt $inc 126.4 294.55 $plane 
set n70 [$ns node]; $n70 set-position $alt $inc 126.4 327.27 $plane 

incr plane
set n75 [$ns node]; $n75 set-position $alt $inc 158 16.36 $plane 
set n76 [$ns node]; $n76 set-position $alt $inc 158 49.09 $plane 
set n77 [$ns node]; $n77 set-position $alt $inc 158 81.82 $plane 
set n78 [$ns node]; $n78 set-position $alt $inc 158 114.55 $plane 
set n79 [$ns node]; $n79 set-position $alt $inc 158 147.27 $plane 
set n80 [$ns node]; $n80 set-position $alt $inc 158 180 $plane 
set n81 [$ns node]; $n81 set-position $alt $inc 158 212.73 $plane 
set n82 [$ns node]; $n82 set-position $alt $inc 158 245.45 $plane 
set n83 [$ns node]; $n83 set-position $alt $inc 158 278.18 $plane 
set n84 [$ns node]; $n84 set-position $alt $inc 158 310.91 $plane 
set n85 [$ns node]; $n85 set-position $alt $inc 158 343.64 $plane 

# By setting the next_ variable on polar sats; handoffs can be optimized

$n0 set_next $n10; $n1 set_next $n0; $n2 set_next $n1; $n3 set_next $n2
$n4 set_next $n3; $n5 set_next $n4; $n6 set_next $n5; $n7 set_next $n6
$n8 set_next $n7; $n9 set_next $n8; $n10 set_next $n9

$n15 set_next $n25; $n16 set_next $n15; $n17 set_next $n16; $n18 set_next $n17
$n19 set_next $n18; $n20 set_next $n19; $n21 set_next $n20; $n22 set_next $n21
$n23 set_next $n22; $n24 set_next $n23; $n25 set_next $n24

$n30 set_next $n40; $n31 set_next $n30; $n32 set_next $n31; $n33 set_next $n32
$n34 set_next $n33; $n35 set_next $n34; $n36 set_next $n35; $n37 set_next $n36
$n38 set_next $n37; $n39 set_next $n38; $n40 set_next $n39

$n45 set_next $n55; $n46 set_next $n45; $n47 set_next $n46; $n48 set_next $n47
$n49 set_next $n48; $n50 set_next $n49; $n51 set_next $n50; $n52 set_next $n51
$n53 set_next $n52; $n54 set_next $n53; $n55 set_next $n54

$n60 set_next $n70; $n61 set_next $n60; $n62 set_next $n61; $n63 set_next $n62
$n64 set_next $n63; $n65 set_next $n64; $n66 set_next $n65; $n67 set_next $n66
$n68 set_next $n67; $n69 set_next $n68; $n70 set_next $n69

$n75 set_next $n85; $n76 set_next $n75; $n77 set_next $n76; $n78 set_next $n77
$n79 set_next $n78; $n80 set_next $n79; $n81 set_next $n80; $n82 set_next $n81
$n83 set_next $n82; $n84 set_next $n83; $n85 set_next $n84

