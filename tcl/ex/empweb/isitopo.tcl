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

#
# Maintainer: Kun-chan Lan <kclan@isi.edu>
# Version Date: $Date: 2002/01/29 01:03:01 $
#
#
# Unbalanced dumbell topology
# Used by isigen.tcl
#
#         ISI hosts      remote hosts
#
#      [ISI clients]
#                        
#       s+11 s+10                2 3 4 [2,3,4,5: web servers]
#          \ |                    \|/ 
#            1 ---9------------X---0 --6-[web clients]
#          / |  [isi svr]         /|\
#        s+8 s+9                 5 7 8
#                                  |  \
#                                  |  [ftp clients]
#                               [ftp server]
#

proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {

       $ns duplex-link $n1 $n2 $bw $delay $queue_method
       $ns queue-limit $n1 $n2 $queue_length
       $ns queue-limit $n2 $n1 $queue_length
}

proc create_topology {} {
global ns n verbose num_node num_nonisi_web_client num_isi_server num_ftp_client

set num_web_server 40        
set num_web_client 960      
set num_isi_client 160     
set num_isi_server 1           
set num_isi_lan 4
set num_ftp_server 10    
set num_ftp_client 100     
set queue_method RED
set queue_length 50
set num_nonisi_web_client [expr $num_web_client - $num_isi_client]
set num_node [expr 15 + [expr $num_web_client + $num_web_server + $num_ftp_server + $num_ftp_client]]
set num_isi_client_per_lan [expr $num_isi_client/$num_isi_lan]

set wwwInDelay [new RandomVariable/Empirical]
$wwwInDelay loadCDF cdf/2pm.dump.www.inbound.delay.cdf
set wwwOutDelay [new RandomVariable/Empirical]
$wwwOutDelay loadCDF cdf/2pm.dump.www.outbound.delay.cdf
set WWWinBW [new RandomVariable/Empirical]
$WWWinBW loadCDF cdf/2pm.dump.www.inbound.BW.cdf
set WWWoutBW [new RandomVariable/Empirical]
$WWWoutBW loadCDF cdf/2pm.dump.www.outbound.BW.cdf

set ftpInDelay [new RandomVariable/Empirical]
$ftpInDelay loadCDF cdf/2pm.dump.ftp.inbound.delay.cdf
set ftpOutDelay [new RandomVariable/Empirical]
$ftpOutDelay loadCDF cdf/2pm.dump.ftp.outbound.delay.cdf
set FTPinBW [new RandomVariable/Empirical]
$FTPinBW loadCDF cdf/2pm.dump.ftp.inbound.BW.cdf
set FTPoutBW [new RandomVariable/Empirical]
$FTPoutBW loadCDF cdf/2pm.dump.ftp.outbound.BW.cdf

if {$verbose} { puts "Creating dumbell topology..." }

for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
}

# EDGES (from-node to-node length a b):
# node 0 connects to the non-isi server and client pools
# node 9 is the isi web/ftp server
# node 1 connects to the isi client pool
my-duplex-link $ns $n(0) $n([expr $num_node - 1]) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n([expr $num_node - 1]) $n(9) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n(9) $n(1) 100Mb 0.5ms $queue_method $queue_length

my-duplex-link $ns $n(0) $n(2) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(3) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(4) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(5) 50Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(6) 2Mb 10ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(7) 100Mb 2ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(8) 2Mb 10ms $queue_method $queue_length

set remote_host [expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client]

my-duplex-link $ns $n(1) $n([expr $remote_host + 10]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $remote_host + 11]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $remote_host + 12]) 100Mb 500us $queue_method $queue_length
my-duplex-link $ns $n(1) $n([expr $remote_host + 13]) 100Mb 500us $queue_method $queue_length

for {set i 0} {$i < $num_web_server} {incr i} {
    set base [expr $i / 10]
    set delay [$wwwInDelay value]
    set bandwidth [$WWWinBW value]

    my-duplex-link $ns $n([expr $base + 2]) $n([expr $i + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n([expr $base + 2]) \$n([expr $i + 10]) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}

    #setup manual routes

    [$n([expr $i + 10]) get-module "Manual"] add-route-to-adj-node -default $n([expr $base + 2])

    [$n([expr $remote_host + 10]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $remote_host + 10]) $n(1)] head]
    [$n([expr $remote_host + 11]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $remote_host + 11]) $n(1)] head]
    [$n([expr $remote_host + 12]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $remote_host + 12]) $n(1)] head]
    [$n([expr $remote_host + 13]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $remote_host + 13]) $n(1)] head]

    [$n(1) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n(1) $n(9)] head]
    [$n(9) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n(9) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $num_node - 1]) $n(0)] head]
    [$n(0) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n(0) $n([expr $base + 2])] head]
    [$n([expr $base + 2]) get-module "Manual"] add-route [$n([expr $i + 10]) set address_]  [[$ns link $n([expr $base + 2]) $n([expr $i + 10])] head]


}
if {$verbose} {puts "done creating web server"}


for {set i 0} {$i < $num_nonisi_web_client} {incr i} {
    	set delay [$wwwOutDelay value]
    	set bandwidth [$WWWoutBW value]

    	set c [expr $i + $num_web_server + 10]
	
    	my-duplex-link $ns $n(6) $n($c) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(6) \$n($c) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}

    #setup manual routes

    [$n($c) get-module "Manual"] add-route-to-adj-node -default $n(6)

    [$n(9) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(9) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n([expr $num_node - 1]) $n(0)] head]
    [$n(0) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(0) $n(6)] head]
    [$n(6) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(6) $n($c)] head]

}
if {$verbose} {puts "done creating non-isi web clients"}




for {set i 0} {$i < $num_ftp_server} {incr i} {
    	set delay [$ftpInDelay value]
    	set bandwidth [$FTPinBW value]

    	set s [expr $i + $num_web_server + $num_nonisi_web_client + 10]

    	my-duplex-link $ns $n(7) $n($s) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(7) \$n($s) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}

    #setup manual routes

    [$n($s) get-module "Manual"] add-route-to-adj-node -default $n(7)

    [$n([expr $remote_host + 10]) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n([expr $remote_host + 10]) $n(1)] head]
    [$n([expr $remote_host + 11]) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n([expr $remote_host + 11]) $n(1)] head]
    [$n([expr $remote_host + 12]) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n([expr $remote_host + 12]) $n(1)] head]
    [$n([expr $remote_host + 13]) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n([expr $remote_host + 13]) $n(1)] head]

    [$n(1) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n(1) $n(9)] head]
    [$n(9) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n(9) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n([expr $num_node - 1]) $n(0)] head]
    [$n(0) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n(0) $n(7)] head]
    [$n(7) get-module "Manual"] add-route [$n($s) set address_]  [[$ns link $n(7) $n($s)] head]
}
if {$verbose} {puts "done creating ftp servers"}


for {set i 0} {$i < $num_ftp_client} {incr i} {
    	set delay [$ftpOutDelay value]
    	set bandwidth [$FTPoutBW value]

    	set c [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + 10]
	
    	my-duplex-link $ns $n(8) $n($c) [expr $bandwidth * 1000000 ] [expr $delay * 0.001] $queue_method $queue_length
    	if {$verbose} {puts "\$ns duplex-link \$n(8) \$n($c) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}

    #setup manual routes

    [$n($c) get-module "Manual"] add-route-to-adj-node -default $n(8)

    [$n(9) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(9) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n([expr $num_node - 1]) $n(0)] head]
    [$n(0) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(0) $n(8)] head]
    [$n(8) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(8) $n($c)] head]
}
if {$verbose} {puts "done creating ftp clients"}



for {set i 0} {$i < $num_isi_client} {incr i} {
#    set base [expr $i / 10]
    set base [expr $i / $num_isi_client_per_lan]
    set delay [uniform 0.5 1.0]
    set bandwidth 10.0

    set b [expr $base + [expr $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client + 10]]
    set c [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]

    my-duplex-link $ns $n($b) $n($c) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method $queue_length
    if {$verbose} {puts "\$ns duplex-link \$n($b) \$n($c) [expr $bandwidth * 1000000] [expr $delay * 0.001] $queue_method"}

    #setup manual routes

    [$n($c) get-module "Manual"] add-route-to-adj-node -default $n($b)

    [$n(2) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(2) $n(0)] head]
    [$n(3) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(3) $n(0)] head]
    [$n(4) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(4) $n(0)] head]
    [$n(5) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(5) $n(0)] head]
    [$n(7) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(7) $n(0)] head]

    [$n(0) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(0) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n([expr $num_node - 1]) $n(9)] head]
    [$n(9) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(9) $n(1)] head]
    [$n(1) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n(1) $n($b)] head]
    [$n($b) get-module "Manual"] add-route [$n($c) set address_]  [[$ns link $n($b) $n($c)] head]
}
if {$verbose} {puts "done creating isi clients"}

#route to ISI server

    [$n(6) get-module "Manual"] add-route [$n(9) set address_]  [[$ns link $n(6) $n(0)] head]
    [$n(8) get-module "Manual"] add-route [$n(9) set address_]  [[$ns link $n(8) $n(0)] head]
    [$n(0) get-module "Manual"] add-route [$n(9) set address_]  [[$ns link $n(0) $n([expr $num_node - 1])] head]
    [$n([expr $num_node - 1]) get-module "Manual"] add-route [$n(9) set address_]  [[$ns link $n([expr $num_node - 1]) $n(9)] head]

$ns set dstW_ "";  #define list of web servers
for {set i 0} {$i <= $num_web_server} {incr i} {
    $ns set dstW_ "[$ns set dstW_] [list [expr $i + 9]]"
}
if {$verbose} {puts "WWW server set: [$ns set dstW_]"}

$ns set srcW_ "";  #define list of web clients 
for {set i 0} {$i < $num_nonisi_web_client} {incr i} {
    $ns set srcW_ "[$ns set srcW_] [list [expr [expr $i + $num_web_server ] + 10]]"
}
for {set i 0} {$i < $num_isi_client} {incr i} {
    $ns set srcW_ "[$ns set srcW_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]]"
}
if {$verbose} {puts "WWW client set: [$ns set srcW_]"}




$ns set dstF_ "";  #define list of ftp servers
for {set i 0} {$i <  1} {incr i} {
    $ns set dstF_ "[$ns set dstF_] [list [expr $i + 9]]"
}
for {set i 0} {$i < $num_ftp_server} {incr i} {
    $ns set dstF_ "[$ns set dstF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client ] + 10]]"
}
if {$verbose} {puts "FTP server set: [$ns set dstF_]"}

$ns set srcF_ "";  #define list of ftp clients 
for {set i 0} {$i < $num_ftp_client} {incr i} {
    $ns set srcF_ "[$ns set srcF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server ] + 10]]"
}
for {set i 0} {$i < $num_isi_client} {incr i} {
    $ns set srcF_ "[$ns set srcF_] [list [expr [expr $i + $num_web_server + $num_nonisi_web_client + $num_ftp_server + $num_ftp_client] + 14]]"
}
if {$verbose} {puts "FTP client set: [$ns set srcF_]"}






if {$verbose} { puts "Finished creating topology..." }
}
# end of create_topology

