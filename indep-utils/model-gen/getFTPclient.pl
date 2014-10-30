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
# An perl script that seperate inbound and outbound traffic of ISI domain, 
# used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#


$ftp_port=21;
$num_ftp=0;


while (<>)  {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);
#        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);


#        $dummy0="";
        $dummy1="";
        $dummy2="";

	$time1=0;
	$time2=0;

	$src=join(".",$ip11,$ip12,$ip13,$ip14);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24);

        if ($srcPort eq $ftp_port) {
	   	$ftpc=$dst;
        } elsif ($dstPort eq $ftp_port) {
	   	$ftpc=$src;
	} else {
		print "Something is wrong!!\n";
	}

	$found=0;
	foreach $j (0 .. $#ftpC) {
		if ($ftpc eq $ftpC[$j]) {
	     		$found=1;
	  	}
	}

	if ($found eq 0) {
		$ftpC[$num_ftp]=$ftpc;
		$num_ftp++;
	}

}

foreach $j (0 .. $#ftpC) {
	print "$ftpC[$j]\n";
}
