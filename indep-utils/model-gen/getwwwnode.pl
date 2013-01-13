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

open(FCLIENT,"> webCLIENT");
open(FSERVER,"> webSERVER");

$wc=0;
$ws=0;

while (<>) {
        ($time,$client,$port,$dummy1, $dummy2, $tt, $dummy3, $dummy4,$server,$rest)= split(/[\n ]/,$_);
#        ($time,$client,$port,$dummy1, $dummy2, $dummy3,$server1,$server2,$rest)= split(/[\n ]/,$_);

	$dummy1="";
	$dummy2="";
	$time="";
	$port="";
	$rest="";

      	if ($dummy3 eq ">") {
		$server=$server1;
        }
      	elsif ($server1 eq ">") {
		$server=$server2;
        }
   	else {
		$server="";
    	}


      	if ($client ne "") {
         	$wclient[$wc]=$client;
         	$wc++;
	}
       	if ($server ne "") {
         	$wserver[$ws]=$server;
         	$ws++;
	}
}

#@clientsort = sort numerically @wclient;
#@serversort = sort numerically @wserver;
@clientsort = sort @wclient;
@serversort = sort @wserver;

$prev="";

foreach $j (0 .. $#clientsort) {
        if ($clientsort[$j] ne $prev) {
            	print FCLIENT "$clientsort[$j]\n";	
	}
        $prev=$clientsort[$j];
}

$prev="";

foreach $j (0 .. $#serversort) {
        if ($serversort[$j] ne $prev) {
            	print FSERVER "$serversort[$j]\n";	
	}
        $prev=$serversort[$j];
}

close(FCLIENT);
close(FSERVER);

#sub numerically { $a ne $b; }
