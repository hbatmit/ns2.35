#!/usr/bin/perl -w

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
# An tcl script that extracts DATA/ACK packets from/to ISI domain, 
# used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

sub usage {
        print STDERR <<END;
	usage: $0 [-s DomainPrefix] [-p Port] 
	Options:
	    -s string  specify IP prefix to distinguish Inbound from outbound
	               traffic (eg. 192.1)
	    -p string  specify the port number

END
exit 1;
}
BEGIN {
    	$dblibdir = "./";
	push(@INC, $dblibdir);
}
use DbGetopt;
require "dblib.pl";
my(@orig_argv) = @ARGV;
&usage if ($#ARGV < 0);
my($prog) = &progname;
my($dbopts) = new DbGetopt("s:p:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 's') {
                $prefix = $dbopts->optarg;
        } elsif ($ch eq 'p') {
                $port = $dbopts->optarg;
        } else {
                &usage;
        };
};

($ip1,$ip2,$ip3,$ip4,$m1,$m2,$m3,$m4) = split(/[.\/ ]/,$prefix);
$ip3="";
$ip4="";
$m1="";
$m2="";
$m3="";
$m4="";


$isiPrefix=join(".",$ip1,$ip2,$port);
#$isiPrefix=join(".",$prefix,$port);

open(OUT,"> outbound.seq") || die("cannot open outbound.seq\n");
open(IN,"> inbound.seq") || die("cannot open inbound.seq\n");

open(OUTP,"> outbound.pkt.size") || die("cannot open outbound.pkt.size\n");
open(INP,"> inbound.pkt.size") || die("cannot open inbound.pkt.size\n");


while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2,$flag,$seqb,$seqe,$size,$size1) = split(/[.:() ]/,$_);
#        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2,$flag,$seqb,$seqe,$size,$size1) = split(/[.:() ]/,$_);


	$sTime=join(".",$time1,$time2);
#	$dummy0="";
	$dummy1="";
	$dummy2="";
	$flag="";

	if ( ($seqe ne "ack") && ($seqb eq "")) {
	   $seqb=$seqe;
	   $seqe=$size;
	   $size=$size1;
	}

	$prefixc=join(".",$ip11,$ip12,$srcPort);
	$prefixs=join(".",$ip21,$ip22,$dstPort);
	$client=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	$server=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);

        if ( $srcPort eq $port ) {
		$server=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	 	$client=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);
        }

        #capture the packet size distribution
	if ( $seqe ne "ack" ) {
		if ( $prefixc eq $isiPrefix) {
			print OUTP "$client $server $sTime $size\n";
		} else {
			print INP "$client $server $sTime $size\n";
		}
	}	

        #capture 3 types of packaets
	# 1. data packet to the server
	# 2. data packet from the server
	# 3. ack packet to the server

        #data packet from ISI
	if ( $prefixc eq $isiPrefix) {

		if ( $seqe ne "ack" ) {
#			if ( $size eq 1460 ) {
			if ( $size > 1400 ) {
				print OUT "$client $server $seqe $sTime data\n"
			}	
                }
	}
	#ACK packet to ISI
	if (($prefixs eq $isiPrefix) && ($seqe eq "ack")) {   
		print OUT "$client $server $size $sTime ack\n"
	}
        #data packet to ISI
        if ( ($srcPort eq $port) && ($prefixc ne $isiPrefix)) {
		if ( $seqe ne "ack" ) {
#			if ( $size eq 1460 ) {
			if ( $size > 1400 ) {
				print IN "$client $server $sTime $seqe\n";
			}
                }
	}
}

close(OUT);
close(IN);
close(OUTP);
close(INP);
