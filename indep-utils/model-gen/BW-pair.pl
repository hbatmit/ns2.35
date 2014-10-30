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
# An tcl script that takes output of BW-seq.tcl and search for in each flow
# pair of DATA/ACK packets which have the same sequence number 
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

$clientd="";
$serverd="";
$seqd="";
$timed=0;

while (<>) {
       
	($client,$server,$seq,$time,$flag) = split(' ',$_);

        if ( $flag eq "data") {
		$clientd=$client;
		$serverd=$server;
		$seqd=$seq;
		$timed=$time;
	}
        if ( $flag eq "ack") {
		if ( ($clientd eq $client) && ($serverd eq $server) && ($seqd eq $seq))  {
			print "$client $server $timed $time $seqd\n"
		}
		$clientd="";
		$serverd="";
		$seqd="";
		$timed=0;
	}

}
