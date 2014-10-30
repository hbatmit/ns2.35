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

if {![info exists ns]} {
	puts "Error:  sat-iridium-links.tcl is a supporting script for the "
	puts "        sat-iridium.tcl script-- run `sat-iridium.tcl' instead"
	exit
}

# Now that the positions are set up, configure the ISLs
# Plane 1 intraplane
$ns add-isl intraplane $n0 $n1 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n1 $n2 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n2 $n3 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n3 $n4 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n4 $n5 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n5 $n6 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n6 $n7 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n7 $n8 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n8 $n9 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n9 $n10 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n10 $n0 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 2 intraplane
$ns add-isl intraplane $n15 $n16 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n16 $n17 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n17 $n18 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n18 $n19 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n19 $n20 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n20 $n21 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n21 $n22 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n22 $n23 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n23 $n24 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n24 $n25 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n25 $n15 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 3 intraplane
$ns add-isl intraplane $n30 $n31 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n31 $n32 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n32 $n33 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n33 $n34 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n34 $n35 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n35 $n36 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n36 $n37 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n37 $n38 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n38 $n39 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n39 $n40 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n40 $n30 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 4 intraplane
$ns add-isl intraplane $n45 $n46 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n46 $n47 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n47 $n48 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n48 $n49 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n49 $n50 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n50 $n51 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n51 $n52 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n52 $n53 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n53 $n54 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n54 $n55 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n55 $n45 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 5 intraplane
$ns add-isl intraplane $n60 $n61 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n61 $n62 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n62 $n63 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n63 $n64 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n64 $n65 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n65 $n66 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n66 $n67 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n67 $n68 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n68 $n69 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n69 $n70 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n70 $n60 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 6 intraplane
$ns add-isl intraplane $n75 $n76 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n76 $n77 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n77 $n78 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n78 $n79 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n79 $n80 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n80 $n81 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n81 $n82 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n82 $n83 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n83 $n84 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n84 $n85 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl intraplane $n85 $n75 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Interplane ISLs
# 2 interplane ISLs per satellite (one along the seam)

# Plane 1-2
$ns add-isl interplane $n0 $n15 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n1 $n16 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n2 $n17 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n3 $n18 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n4 $n19 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n5 $n20 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n6 $n21 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n7 $n22 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n8 $n23 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n9 $n24 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n10 $n25 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 2-3
$ns add-isl interplane $n15 $n30 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n16 $n31 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n17 $n32 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n18 $n33 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n19 $n34 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n20 $n35 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n21 $n36 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n22 $n37 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n23 $n38 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n24 $n39 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n25 $n40 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 3-4
$ns add-isl interplane $n30 $n45 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n31 $n46 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n32 $n47 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n33 $n48 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n34 $n49 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n35 $n50 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n36 $n51 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n37 $n52 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n38 $n53 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n39 $n54 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n40 $n55 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 4-5
$ns add-isl interplane $n45 $n60 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n46 $n61 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n47 $n62 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n48 $n63 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n49 $n64 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n50 $n65 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n51 $n66 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n52 $n67 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n53 $n68 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n54 $n69 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n55 $n70 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Plane 5-6
$ns add-isl interplane $n60 $n75 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n61 $n76 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n62 $n77 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n63 $n78 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n64 $n79 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n65 $n80 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n66 $n81 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n67 $n82 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n68 $n83 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n69 $n84 $opt(bw_isl) $opt(ifq) $opt(qlim)
$ns add-isl interplane $n70 $n85 $opt(bw_isl) $opt(ifq) $opt(qlim)

# Cross-seam 
# To run the topology with crossseam ISLs, uncomment the remaining lines
# 
#$ns add-isl crossseam $n0 $n80 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n1 $n79 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n2 $n78 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n3 $n77 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n4 $n76 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n5 $n75 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n6 $n85 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n7 $n84 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n8 $n83 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n9 $n82 $opt(bw_isl) $opt(ifq) $opt(qlim)
#$ns add-isl crossseam $n10 $n81 $opt(bw_isl) $opt(ifq) $opt(qlim)
