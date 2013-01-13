#
# Copyright (C) 1999 by USC/ISI
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

#
# Maintained by: Polly Huang Tue Feb  2 14:34:54 PST 1999
# Version Date: $Date: 1999/04/20 22:34:30 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/shuttle.tcl,v 1.2 1999/04/20 22:34:30 polly Exp $ (USC/ISI)
#
# 
# creating mbone shuttle session topology 
# generated from 1996 MBone trace

proc create-topology {} {
global ns n verbose num_node
set num_node 255
set verbose 1
if {$verbose} { puts "Creating 255 nodes..."; flush stdout }

for {set i 0} {$i < 255} {incr i} {
        set n($i) [$ns node]
}
if {$verbose} { puts "Creating links..."; flush stdout }
$ns duplex-link $n(0) $n(1) 512000.000000 0.010000 DropTail
$ns duplex-link $n(1) $n(124) 512000.000000 0.010000 DropTail
$ns duplex-link $n(1) $n(2) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(125) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(3) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(4) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(5) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(6) 512000.000000 0.010000 DropTail
$ns duplex-link $n(2) $n(7) 512000.000000 0.010000 DropTail
$ns duplex-link $n(4) $n(10) 512000.000000 0.010000 DropTail
$ns duplex-link $n(4) $n(128) 512000.000000 0.010000 DropTail
$ns duplex-link $n(4) $n(8) 512000.000000 0.010000 DropTail
$ns duplex-link $n(4) $n(9) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(11) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(12) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(127) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(13) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(14) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(15) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(16) 512000.000000 0.010000 DropTail
$ns duplex-link $n(5) $n(182) 512000.000000 0.010000 DropTail
$ns duplex-link $n(6) $n(17) 512000.000000 0.010000 DropTail
$ns duplex-link $n(6) $n(18) 512000.000000 0.010000 DropTail
$ns duplex-link $n(6) $n(19) 512000.000000 0.010000 DropTail
$ns duplex-link $n(7) $n(20) 512000.000000 0.010000 DropTail
$ns duplex-link $n(10) $n(214) 512000.000000 0.010000 DropTail
$ns duplex-link $n(11) $n(197) 512000.000000 0.010000 DropTail
$ns duplex-link $n(11) $n(21) 512000.000000 0.010000 DropTail
$ns duplex-link $n(12) $n(132) 512000.000000 0.010000 DropTail
$ns duplex-link $n(12) $n(22) 512000.000000 0.010000 DropTail
$ns duplex-link $n(12) $n(23) 512000.000000 0.010000 DropTail
$ns duplex-link $n(12) $n(24) 512000.000000 0.010000 DropTail
$ns duplex-link $n(13) $n(131) 512000.000000 0.010000 DropTail
$ns duplex-link $n(13) $n(25) 512000.000000 0.010000 DropTail
$ns duplex-link $n(13) $n(26) 512000.000000 0.010000 DropTail
$ns duplex-link $n(14) $n(133) 512000.000000 0.010000 DropTail
$ns duplex-link $n(14) $n(160) 512000.000000 0.010000 DropTail
$ns duplex-link $n(14) $n(216) 512000.000000 0.010000 DropTail
$ns duplex-link $n(14) $n(27) 512000.000000 0.010000 DropTail
$ns duplex-link $n(14) $n(28) 512000.000000 0.010000 DropTail
$ns duplex-link $n(15) $n(161) 512000.000000 0.010000 DropTail
$ns duplex-link $n(15) $n(29) 512000.000000 0.010000 DropTail
$ns duplex-link $n(15) $n(30) 512000.000000 0.010000 DropTail
$ns duplex-link $n(16) $n(217) 512000.000000 0.010000 DropTail
$ns duplex-link $n(16) $n(31) 512000.000000 0.010000 DropTail
$ns duplex-link $n(17) $n(32) 512000.000000 0.010000 DropTail
$ns duplex-link $n(18) $n(33) 512000.000000 0.010000 DropTail
$ns duplex-link $n(19) $n(34) 512000.000000 0.010000 DropTail
$ns duplex-link $n(21) $n(35) 512000.000000 0.010000 DropTail
$ns duplex-link $n(21) $n(36) 512000.000000 0.010000 DropTail
$ns duplex-link $n(21) $n(37) 512000.000000 0.010000 DropTail
$ns duplex-link $n(22) $n(137) 512000.000000 0.010000 DropTail
$ns duplex-link $n(22) $n(185) 512000.000000 0.010000 DropTail
$ns duplex-link $n(22) $n(207) 512000.000000 0.010000 DropTail
$ns duplex-link $n(22) $n(38) 512000.000000 0.010000 DropTail
$ns duplex-link $n(23) $n(222) 512000.000000 0.010000 DropTail
$ns duplex-link $n(23) $n(39) 512000.000000 0.010000 DropTail
$ns duplex-link $n(24) $n(40) 512000.000000 0.010000 DropTail
$ns duplex-link $n(24) $n(41) 512000.000000 0.010000 DropTail
$ns duplex-link $n(25) $n(221) 512000.000000 0.010000 DropTail
$ns duplex-link $n(25) $n(42) 512000.000000 0.010000 DropTail
$ns duplex-link $n(25) $n(43) 512000.000000 0.010000 DropTail
$ns duplex-link $n(25) $n(44) 512000.000000 0.010000 DropTail
$ns duplex-link $n(26) $n(45) 512000.000000 0.010000 DropTail
$ns duplex-link $n(26) $n(46) 512000.000000 0.010000 DropTail
$ns duplex-link $n(27) $n(163) 512000.000000 0.010000 DropTail
$ns duplex-link $n(27) $n(177) 512000.000000 0.010000 DropTail
$ns duplex-link $n(27) $n(220) 512000.000000 0.010000 DropTail
$ns duplex-link $n(27) $n(47) 512000.000000 0.010000 DropTail
$ns duplex-link $n(29) $n(164) 512000.000000 0.010000 DropTail
$ns duplex-link $n(29) $n(48) 512000.000000 0.010000 DropTail
$ns duplex-link $n(30) $n(49) 512000.000000 0.010000 DropTail
$ns duplex-link $n(31) $n(50) 512000.000000 0.010000 DropTail
$ns duplex-link $n(33) $n(51) 512000.000000 0.010000 DropTail
$ns duplex-link $n(35) $n(52) 512000.000000 0.010000 DropTail
$ns duplex-link $n(36) $n(140) 512000.000000 0.010000 DropTail
$ns duplex-link $n(36) $n(53) 512000.000000 0.010000 DropTail
$ns duplex-link $n(37) $n(54) 512000.000000 0.010000 DropTail
$ns duplex-link $n(38) $n(55) 512000.000000 0.010000 DropTail
$ns duplex-link $n(39) $n(141) 512000.000000 0.010000 DropTail
$ns duplex-link $n(39) $n(56) 512000.000000 0.010000 DropTail
$ns duplex-link $n(40) $n(166) 512000.000000 0.010000 DropTail
$ns duplex-link $n(40) $n(167) 512000.000000 0.010000 DropTail
$ns duplex-link $n(40) $n(230) 512000.000000 0.010000 DropTail
$ns duplex-link $n(40) $n(57) 512000.000000 0.010000 DropTail
$ns duplex-link $n(41) $n(231) 512000.000000 0.010000 DropTail
$ns duplex-link $n(41) $n(58) 512000.000000 0.010000 DropTail
$ns duplex-link $n(43) $n(59) 512000.000000 0.010000 DropTail
$ns duplex-link $n(44) $n(168) 512000.000000 0.010000 DropTail
$ns duplex-link $n(44) $n(189) 512000.000000 0.010000 DropTail
$ns duplex-link $n(44) $n(60) 512000.000000 0.010000 DropTail
$ns duplex-link $n(45) $n(61) 512000.000000 0.010000 DropTail
$ns duplex-link $n(46) $n(178) 512000.000000 0.010000 DropTail
$ns duplex-link $n(46) $n(226) 512000.000000 0.010000 DropTail
$ns duplex-link $n(46) $n(62) 512000.000000 0.010000 DropTail
$ns duplex-link $n(47) $n(208) 512000.000000 0.010000 DropTail
$ns duplex-link $n(47) $n(63) 512000.000000 0.010000 DropTail
$ns duplex-link $n(48) $n(169) 512000.000000 0.010000 DropTail
$ns duplex-link $n(48) $n(233) 512000.000000 0.010000 DropTail
$ns duplex-link $n(49) $n(142) 512000.000000 0.010000 DropTail
$ns duplex-link $n(49) $n(234) 512000.000000 0.010000 DropTail
$ns duplex-link $n(49) $n(64) 512000.000000 0.010000 DropTail
$ns duplex-link $n(50) $n(176) 512000.000000 0.010000 DropTail
$ns duplex-link $n(50) $n(65) 512000.000000 0.010000 DropTail
$ns duplex-link $n(53) $n(236) 512000.000000 0.010000 DropTail
$ns duplex-link $n(55) $n(66) 512000.000000 0.010000 DropTail
$ns duplex-link $n(56) $n(67) 512000.000000 0.010000 DropTail
$ns duplex-link $n(57) $n(198) 512000.000000 0.010000 DropTail
$ns duplex-link $n(57) $n(241) 512000.000000 0.010000 DropTail
$ns duplex-link $n(57) $n(68) 512000.000000 0.010000 DropTail
$ns duplex-link $n(58) $n(69) 512000.000000 0.010000 DropTail
$ns duplex-link $n(59) $n(70) 512000.000000 0.010000 DropTail
$ns duplex-link $n(61) $n(238) 512000.000000 0.010000 DropTail
$ns duplex-link $n(61) $n(71) 512000.000000 0.010000 DropTail
$ns duplex-link $n(62) $n(72) 512000.000000 0.010000 DropTail
$ns duplex-link $n(63) $n(237) 512000.000000 0.010000 DropTail
$ns duplex-link $n(63) $n(73) 512000.000000 0.010000 DropTail
$ns duplex-link $n(63) $n(85) 512000.000000 0.010000 DropTail
$ns duplex-link $n(64) $n(74) 512000.000000 0.010000 DropTail
$ns duplex-link $n(69) $n(146) 512000.000000 0.010000 DropTail
$ns duplex-link $n(70) $n(75) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(145) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(173) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(245) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(73) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(76) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(77) 512000.000000 0.010000 DropTail
$ns duplex-link $n(72) $n(96) 512000.000000 0.010000 DropTail
$ns duplex-link $n(73) $n(147) 512000.000000 0.010000 DropTail
$ns duplex-link $n(73) $n(78) 512000.000000 0.010000 DropTail
$ns duplex-link $n(75) $n(79) 512000.000000 0.010000 DropTail
$ns duplex-link $n(76) $n(200) 512000.000000 0.010000 DropTail
$ns duplex-link $n(76) $n(80) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(81) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(82) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(83) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(84) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(85) 512000.000000 0.010000 DropTail
$ns duplex-link $n(77) $n(86) 512000.000000 0.010000 DropTail
$ns duplex-link $n(78) $n(87) 512000.000000 0.010000 DropTail
$ns duplex-link $n(79) $n(88) 512000.000000 0.010000 DropTail
$ns duplex-link $n(80) $n(89) 512000.000000 0.010000 DropTail
$ns duplex-link $n(81) $n(245) 512000.000000 0.010000 DropTail
$ns duplex-link $n(82) $n(150) 512000.000000 0.010000 DropTail
$ns duplex-link $n(82) $n(174) 512000.000000 0.010000 DropTail
$ns duplex-link $n(82) $n(90) 512000.000000 0.010000 DropTail
$ns duplex-link $n(83) $n(150) 512000.000000 0.010000 DropTail
$ns duplex-link $n(83) $n(151) 512000.000000 0.010000 DropTail
$ns duplex-link $n(83) $n(152) 512000.000000 0.010000 DropTail
$ns duplex-link $n(83) $n(91) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(150) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(245) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(92) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(93) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(94) 512000.000000 0.010000 DropTail
$ns duplex-link $n(84) $n(95) 512000.000000 0.010000 DropTail
$ns duplex-link $n(85) $n(150) 512000.000000 0.010000 DropTail
$ns duplex-link $n(85) $n(245) 512000.000000 0.010000 DropTail
$ns duplex-link $n(85) $n(96) 512000.000000 0.010000 DropTail
$ns duplex-link $n(85) $n(97) 512000.000000 0.010000 DropTail
$ns duplex-link $n(86) $n(150) 512000.000000 0.010000 DropTail
$ns duplex-link $n(87) $n(98) 512000.000000 0.010000 DropTail
$ns duplex-link $n(89) $n(195) 512000.000000 0.010000 DropTail
$ns duplex-link $n(89) $n(99) 512000.000000 0.010000 DropTail
$ns duplex-link $n(91) $n(100) 512000.000000 0.010000 DropTail
$ns duplex-link $n(92) $n(101) 512000.000000 0.010000 DropTail
$ns duplex-link $n(92) $n(175) 512000.000000 0.010000 DropTail
$ns duplex-link $n(93) $n(102) 512000.000000 0.010000 DropTail
$ns duplex-link $n(93) $n(103) 512000.000000 0.010000 DropTail
$ns duplex-link $n(94) $n(104) 512000.000000 0.010000 DropTail
$ns duplex-link $n(95) $n(105) 512000.000000 0.010000 DropTail
$ns duplex-link $n(95) $n(193) 512000.000000 0.010000 DropTail
$ns duplex-link $n(96) $n(106) 512000.000000 0.010000 DropTail
$ns duplex-link $n(96) $n(180) 512000.000000 0.010000 DropTail
$ns duplex-link $n(96) $n(211) 512000.000000 0.010000 DropTail
$ns duplex-link $n(98) $n(107) 512000.000000 0.010000 DropTail
$ns duplex-link $n(99) $n(108) 512000.000000 0.010000 DropTail
$ns duplex-link $n(99) $n(109) 512000.000000 0.010000 DropTail
$ns duplex-link $n(100) $n(110) 512000.000000 0.010000 DropTail
$ns duplex-link $n(101) $n(111) 512000.000000 0.010000 DropTail
$ns duplex-link $n(103) $n(112) 512000.000000 0.010000 DropTail
$ns duplex-link $n(103) $n(113) 512000.000000 0.010000 DropTail
$ns duplex-link $n(103) $n(155) 512000.000000 0.010000 DropTail
$ns duplex-link $n(103) $n(156) 512000.000000 0.010000 DropTail
$ns duplex-link $n(104) $n(114) 512000.000000 0.010000 DropTail
$ns duplex-link $n(104) $n(204) 512000.000000 0.010000 DropTail
$ns duplex-link $n(105) $n(115) 512000.000000 0.010000 DropTail
$ns duplex-link $n(105) $n(116) 512000.000000 0.010000 DropTail
$ns duplex-link $n(108) $n(117) 512000.000000 0.010000 DropTail
$ns duplex-link $n(110) $n(157) 512000.000000 0.010000 DropTail
$ns duplex-link $n(113) $n(118) 512000.000000 0.010000 DropTail
$ns duplex-link $n(114) $n(119) 512000.000000 0.010000 DropTail
$ns duplex-link $n(116) $n(120) 512000.000000 0.010000 DropTail
$ns duplex-link $n(118) $n(121) 512000.000000 0.010000 DropTail
$ns duplex-link $n(119) $n(122) 512000.000000 0.010000 DropTail
$ns duplex-link $n(119) $n(159) 512000.000000 0.010000 DropTail
$ns duplex-link $n(119) $n(252) 512000.000000 0.010000 DropTail
$ns duplex-link $n(122) $n(123) 512000.000000 0.010000 DropTail
$ns duplex-link $n(122) $n(196) 512000.000000 0.010000 DropTail
$ns duplex-link $n(125) $n(126) 512000.000000 0.010000 DropTail
$ns duplex-link $n(126) $n(129) 512000.000000 0.010000 DropTail
$ns duplex-link $n(127) $n(130) 512000.000000 0.010000 DropTail
$ns duplex-link $n(130) $n(134) 512000.000000 0.010000 DropTail
$ns duplex-link $n(130) $n(135) 512000.000000 0.010000 DropTail
$ns duplex-link $n(132) $n(136) 512000.000000 0.010000 DropTail
$ns duplex-link $n(132) $n(186) 512000.000000 0.010000 DropTail
$ns duplex-link $n(132) $n(187) 512000.000000 0.010000 DropTail
$ns duplex-link $n(133) $n(138) 512000.000000 0.010000 DropTail
$ns duplex-link $n(134) $n(139) 512000.000000 0.010000 DropTail
$ns duplex-link $n(140) $n(143) 512000.000000 0.010000 DropTail
$ns duplex-link $n(140) $n(235) 512000.000000 0.010000 DropTail
$ns duplex-link $n(142) $n(144) 512000.000000 0.010000 DropTail
$ns duplex-link $n(145) $n(148) 512000.000000 0.010000 DropTail
$ns duplex-link $n(145) $n(201) 512000.000000 0.010000 DropTail
$ns duplex-link $n(145) $n(248) 512000.000000 0.010000 DropTail
$ns duplex-link $n(147) $n(149) 512000.000000 0.010000 DropTail
$ns duplex-link $n(147) $n(247) 512000.000000 0.010000 DropTail
$ns duplex-link $n(152) $n(153) 512000.000000 0.010000 DropTail
$ns duplex-link $n(152) $n(194) 512000.000000 0.010000 DropTail
$ns duplex-link $n(153) $n(154) 512000.000000 0.010000 DropTail
$ns duplex-link $n(155) $n(158) 512000.000000 0.010000 DropTail
$ns duplex-link $n(160) $n(162) 512000.000000 0.010000 DropTail
$ns duplex-link $n(161) $n(164) 512000.000000 0.010000 DropTail
$ns duplex-link $n(163) $n(165) 512000.000000 0.010000 DropTail
$ns duplex-link $n(164) $n(169) 512000.000000 0.010000 DropTail
$ns duplex-link $n(164) $n(232) 512000.000000 0.010000 DropTail
$ns duplex-link $n(166) $n(170) 512000.000000 0.010000 DropTail
$ns duplex-link $n(167) $n(171) 512000.000000 0.010000 DropTail
$ns duplex-link $n(171) $n(172) 512000.000000 0.010000 DropTail
$ns duplex-link $n(178) $n(179) 512000.000000 0.010000 DropTail
$ns duplex-link $n(180) $n(181) 512000.000000 0.010000 DropTail
$ns duplex-link $n(182) $n(183) 512000.000000 0.010000 DropTail
$ns duplex-link $n(182) $n(215) 512000.000000 0.010000 DropTail
$ns duplex-link $n(183) $n(184) 512000.000000 0.010000 DropTail
$ns duplex-link $n(184) $n(188) 512000.000000 0.010000 DropTail
$ns duplex-link $n(186) $n(190) 512000.000000 0.010000 DropTail
$ns duplex-link $n(187) $n(191) 512000.000000 0.010000 DropTail
$ns duplex-link $n(188) $n(192) 512000.000000 0.010000 DropTail
$ns duplex-link $n(198) $n(199) 512000.000000 0.010000 DropTail
$ns duplex-link $n(201) $n(202) 512000.000000 0.010000 DropTail
$ns duplex-link $n(202) $n(203) 512000.000000 0.010000 DropTail
$ns duplex-link $n(203) $n(205) 512000.000000 0.010000 DropTail
$ns duplex-link $n(204) $n(206) 512000.000000 0.010000 DropTail
$ns duplex-link $n(207) $n(209) 512000.000000 0.010000 DropTail
$ns duplex-link $n(208) $n(210) 512000.000000 0.010000 DropTail
$ns duplex-link $n(211) $n(212) 512000.000000 0.010000 DropTail
$ns duplex-link $n(212) $n(213) 512000.000000 0.010000 DropTail
$ns duplex-link $n(215) $n(218) 512000.000000 0.010000 DropTail
$ns duplex-link $n(216) $n(219) 512000.000000 0.010000 DropTail
$ns duplex-link $n(217) $n(223) 512000.000000 0.010000 DropTail
$ns duplex-link $n(217) $n(224) 512000.000000 0.010000 DropTail
$ns duplex-link $n(220) $n(225) 512000.000000 0.010000 DropTail
$ns duplex-link $n(221) $n(227) 512000.000000 0.010000 DropTail
$ns duplex-link $n(221) $n(228) 512000.000000 0.010000 DropTail
$ns duplex-link $n(222) $n(229) 512000.000000 0.010000 DropTail
$ns duplex-link $n(229) $n(239) 512000.000000 0.010000 DropTail
$ns duplex-link $n(230) $n(240) 512000.000000 0.010000 DropTail
$ns duplex-link $n(231) $n(242) 512000.000000 0.010000 DropTail
$ns duplex-link $n(232) $n(243) 512000.000000 0.010000 DropTail
$ns duplex-link $n(237) $n(244) 512000.000000 0.010000 DropTail
$ns duplex-link $n(241) $n(246) 512000.000000 0.010000 DropTail
$ns duplex-link $n(247) $n(249) 512000.000000 0.010000 DropTail
$ns duplex-link $n(248) $n(250) 512000.000000 0.010000 DropTail
$ns duplex-link $n(250) $n(251) 512000.000000 0.010000 DropTail
$ns duplex-link $n(252) $n(253) 512000.000000 0.010000 DropTail
$ns duplex-link $n(253) $n(254) 512000.000000 0.010000 DropTail
if {$verbose} { puts "266 links created."; flush stdout }
}
