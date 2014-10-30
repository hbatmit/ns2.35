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
        usage: $0 [-r Filename] [-s DomainPrefix]
	        Options:
	            -r string  file that contains a list of FTP clients, which
	                       is output of getFTPclient.pl
		    -s string  specify IP prefix to distinguish Inbound from 
		               outbound traffic (eg. 192.1)

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
my($dbopts) = new DbGetopt("r:s:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 'r') {
                $infile = $dbopts->optarg;
        } elsif ($ch eq 's') {
		$prefix = $dbopts->optarg;
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


$isiPrefix=join(".",$ip1,$ip2);



open(OUT,"> outbound.seq") || die("cannot open outbound.seq\n");
open(IN,"> inbound.seq") || die("cannot open inbound.seq\n");

open(FIN,"< $infile") || die("cannot open $infile\n");

$num_ftp=0;

while (<FIN>) {
   ($host,$newline) = split(/[:() \n]/,$_);
   $newline="";
   $ftpC[$num_ftp]=$host;
   $num_ftp++;
}

close(FIN);


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

        $prefixc=join(".",$ip11,$ip12);
	$prefixs=join(".",$ip21,$ip22);

	$src=join(".",$ip11,$ip12,$ip13,$ip14);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24);

	$srcp=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
 	$dstp=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);


        $found=0;
        foreach $j (0 .. $#ftpC) {
       		if ($src eq $ftpC[$j]) {
                  	$found=1;
			$client=$srcp;
			$server=$dstp;
                }
       		if ($dst eq $ftpC[$j]) {
                  	$found=1;
			$client=$dstp;
			$server=$srcp;
                }
								                         }
      	if ($found eq 0) { 
		print "ERROR in BW-seq-ftp.pl: $src $dst\n";
 	}			                    

        #capture 3 types of packaets
	# 1. data packet to the server
	# 2. data packet from the server
	# 3. ack packet to the server

        #data packet from ISI server
	if ( $prefixc eq $isiPrefix) {
		
#	if ($srcp eq $server) {
		if ( $seqe ne "ack" ) {
#			if ( $size eq 1460 ) {
			if ( $size >  1000 ) {
				print OUT "$client $server $seqe $sTime data\n"
			}	
                }
	}
	#ACK packet to ISI
	if ($prefixs eq $isiPrefix)  {
		if ($seqe eq "ack") {   
			print OUT "$client $server $size $sTime ack\n"
		}
		else {
#			if ( $size eq 1460 ) {
			if ( $size > 1000 ) {
				print IN "$client $server $sTime $seqe\n";
			}
                }
	}

}
close(OUT);
close(IN);
