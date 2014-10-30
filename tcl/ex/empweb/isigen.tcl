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
# An example script that simulates ISI web traffic. 
#
# Some attributes:
# 1. Topology: ~1100 nodes, 960 web clients, 40 web servers
#                           100 ftp clients, 10 ftp servers
# 2. Traffic: approximately 1,700,000 packets, heavy-tailed connection 
#             sizes, throughout 3600 second simulation time
# 3. Simulation scale: ~33 MB memory, ~1 hrs running on Red Hat Linux 7.0
#              Pentium II Xeon 450 MHz PC with 1GB physical memory
#
# Created by Kun-chan Lan (kclang@isi.edu)

global num_node n verbose num_ftp_client num_nonisi_web_client num_isi_server 
set verbose 1
set enableWEB 1
set enableFTP 0

source isitopo.tcl

# Basic ns setup
set ns [new Simulator]

$ns rtproto Manual

create_topology

#trace files setup (isi: inbound traffic  www: all traffic)
$ns trace-queue $n(9) $n(1) [open isi.in w]
$ns trace-queue $n(1) $n(9) [open isi.out w]
$ns trace-queue $n(9) $n([expr $num_node - 1]) [open www.out w]
$ns trace-queue $n([expr $num_node - 1]) $n(9) [open www.in w]

set stopTime 3600.1

set stopTimeW $stopTime
set stopTimeF $stopTime


# Number of Sessions
set numSessionI 3000
set numSessionO 1500

# Random seed at every run
global defaultRNG
$defaultRNG seed 0

if {$enableWEB ==  1} {

#setup Web model

# Create page poolWWW
set poolWWW [new PagePool/EmpWebTraf]

# Setup web servers and clients
$poolWWW set-num-client [llength [$ns set srcW_]]
$poolWWW set-num-server [llength [$ns set dstW_]]

$poolWWW set-num-remote-client $num_nonisi_web_client
$poolWWW set-num-server-lan $num_isi_server

set i 0
foreach s [$ns set srcW_] {
	$poolWWW set-client $i $n($s)
	incr i
}
set i 0
foreach s [$ns set dstW_] {
	$poolWWW set-server $i $n($s)
	incr i
}



# Inter-session Interval
set WWWinterSessionO [new RandomVariable/Empirical]
$WWWinterSessionO loadCDF cdf/2pm.dump.www.out.sess.inter.cdf
set WWWinterSessionI [new RandomVariable/Empirical]
$WWWinterSessionI loadCDF cdf/2pm.dump.www.in.sess.inter.cdf
#set WWWinterSessionO [new RandomVariable/Exponential]
#$WWWinterSessionO set avg_ 13.6
#set WWWinterSessionI [new RandomVariable/Exponential]
#$WWWinterSessionI set avg_ 1.6

## Number of Pages per Session
set WWWsessionSizeO [new RandomVariable/Empirical]
$WWWsessionSizeO loadCDF cdf/2pm.dump.www.out.pagecnt.cdf
set WWWsessionSizeI [new RandomVariable/Empirical]
$WWWsessionSizeI loadCDF cdf/2pm.dump.www.in.pagecnt.cdf



# Create sessions
$poolWWW set-num-session [expr $numSessionI + $numSessionO]

puts "creating WWW outbound session"

set interPage [new RandomVariable/Empirical]
$interPage loadCDF cdf/2pm.dump.www.out.idle.cdf

set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Empirical]
$interObj loadCDF cdf/2pm.dump.www.out.obj.cdf  

set objSize [new RandomVariable/Empirical]
$objSize loadCDF cdf/2pm.dump.www.out.pagesize.cdf
set reqSize [new RandomVariable/Empirical]
$reqSize loadCDF cdf/2pm.dump.www.out.req.cdf

set persistSel [new RandomVariable/Empirical]
$persistSel loadCDF cdf/persist.cdf
set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/2pm.dump.outbound.server.cdf

set windowS [new RandomVariable/Empirical]
$windowS loadCDF cdf/2pm.dump.www.outbound.wins.cdf
set windowC [new RandomVariable/Empirical]
$windowC loadCDF cdf/2pm.dump.www.outbound.winc.cdf

set mtu [new RandomVariable/Empirical]
$mtu loadCDF cdf/mtu.cdf

set launchTime 0
for {set i 0} {$i < $numSessionO} {incr i} {
        if {$launchTime <=  $stopTimeW} {
	   set numPage [$WWWsessionSizeO value]
	   puts "Session Outbound $i has $numPage pages $launchTime"

	   $poolWWW create-session  $i  \
	                $numPage [expr $launchTime + 0.1] \
			$interPage $pageSize $interObj $objSize \
                        $reqSize $persistSel $serverSel $windowS $windowC $mtu 1

	   set launchTime [expr $launchTime + [$WWWinterSessionO value]]
	}
}

puts "creating WWW inbound session"

set interPage [new RandomVariable/Empirical]
$interPage loadCDF cdf/2pm.dump.www.in.idle.cdf

set pageSize [new RandomVariable/Constant]
$pageSize set val_ 1
set interObj [new RandomVariable/Empirical]
$interObj loadCDF cdf/2pm.dump.www.in.obj.cdf  

set objSize [new RandomVariable/Empirical]
$objSize loadCDF cdf/2pm.dump.www.in.pagesize.cdf
set reqSize [new RandomVariable/Empirical]
$reqSize loadCDF cdf/2pm.dump.www.in.req.cdf

set persistSel [new RandomVariable/Empirical]
$persistSel loadCDF cdf/persist.cdf
set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/2pm.dump.www.inbound.server.cdf

set windowS [new RandomVariable/Empirical]
$windowS loadCDF cdf/2pm.dump.www.inbound.wins.cdf
set windowC [new RandomVariable/Empirical]
$windowC loadCDF cdf/2pm.dump.www.inbound.winc.cdf

set mtu [new RandomVariable/Empirical]
$mtu loadCDF cdf/mtu.cdf

set launchTime 0
for {set i 0} {$i < $numSessionI} {incr i} {
        if {$launchTime <=  $stopTimeW} {
	   set numPage [$WWWsessionSizeI value]
	   puts "Session Inbound $i has $numPage pages $launchTime"

	   $poolWWW create-session [expr $i + $numSessionO] \
	                $numPage [expr $launchTime + 0.1] \
			$interPage $pageSize $interObj $objSize \
                        $reqSize $persistSel $serverSel $windowS $windowC $mtu 0

	   set launchTime [expr $launchTime + [$WWWinterSessionI value]]
        }
}

}


if {$enableFTP ==  1} {

# setup FTP model

# Create page pool
set poolFTP [new PagePool/EmpFtpTraf]

# Setup FTP servers and clients
$poolFTP set-num-client [llength [$ns set srcF_]]
$poolFTP set-num-server [llength [$ns set dstF_]]

$poolFTP set-num-remote-client $num_ftp_client
$poolFTP set-num-server-lan $num_isi_server

set i 0
foreach s [$ns set srcF_] {
        $poolFTP set-client $i $n($s)
        incr i
}
set i 0
foreach s [$ns set dstF_] {
        $poolFTP set-server $i $n($s)
	incr i
}

# Inter-session Interval
set FTPinterSessionO [new RandomVariable/Empirical]
$FTPinterSessionO loadCDF cdf/2pm.dump.ftp.outbound.sess.inter.cdf
set FTPinterSessionI [new RandomVariable/Empirical]
$FTPinterSessionI loadCDF cdf/2pm.dump.ftp.inbound.sess.inter.cdf

## Number of File per Session
set FTPsessionSizeO [new RandomVariable/Empirical]
$FTPsessionSizeO loadCDF cdf/2pm.dump.ftp.outbound.fileno.cdf
set FTPsessionSizeI [new RandomVariable/Empirical]
$FTPsessionSizeI loadCDF cdf/2pm.dump.ftp.inbound.fileno.cdf

$poolFTP set-num-session [expr $numSessionI + $numSessionO]

puts "creating FTP outbound session"

set interFile [new RandomVariable/Empirical]
$interFile loadCDF cdf/2pm.dump.ftp.outbound.file.inter.cdf

set fileSize [new RandomVariable/Empirical]
$fileSize loadCDF cdf/2pm.dump.ftp.outbound.size.cdf

set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/2pm.dump.outbound.server.cdf

set windowS [new RandomVariable/Empirical]
$windowS loadCDF cdf/2pm.dump.ftp.outbound.wins.cdf
set windowC [new RandomVariable/Empirical]
$windowC loadCDF cdf/2pm.dump.ftp.outbound.winc.cdf

set launchTime 0
for {set i 0} {$i < $numSessionO} {incr i} {
        if {$launchTime <=  $stopTimeF} {

	   	set numFile [$FTPsessionSizeO value]
           	puts "Session Outbound $i has $numFile files $launchTime"

       		$poolFTP create-session  $i  \
                         $numFile [expr $launchTime + 0.1] \
		         $interFile $fileSize $serverSel $windowS $windowC 1
           	set launchTime [expr $launchTime + [$FTPinterSessionO value]]
        }
}


puts "creating FTP inbound session"

set interFile [new RandomVariable/Empirical]
$interFile loadCDF cdf/2pm.dump.ftp.inbound.file.inter.cdf

set fileSize [new RandomVariable/Empirical]
$fileSize loadCDF cdf/2pm.dump.ftp.inbound.size.cdf

set serverSel [new RandomVariable/Empirical]
$serverSel loadCDF cdf/2pm.dump.ftp.inbound.server.cdf

set windowS [new RandomVariable/Empirical]
$windowS loadCDF cdf/2pm.dump.ftp.inbound.wins.cdf
set windowC [new RandomVariable/Empirical]
$windowC loadCDF cdf/2pm.dump.ftp.inbound.winc.cdf


set launchTime 0
for {set i 0} {$i < $numSessionI} {incr i} {
        if {$launchTime <=  $stopTimeF} {

	   	set numFile [$FTPsessionSizeI value]
		puts "Session Inbound $i has $numFile files $launchTime"

		$poolFTP create-session [expr $i + $numSessionO] \
                         $numFile [expr $launchTime + 0.1] \
                         $interFile $fileSize $serverSel $windowS $windowC 0

	        set launchTime [expr $launchTime + [$FTPinterSessionI value]]
         }
}

}

## Start the simulation
$ns at $stopTime "finish"

proc finish {} {
	global ns gf 
	$ns flush-trace

	exit 0
}

puts "ns started"
$ns run

