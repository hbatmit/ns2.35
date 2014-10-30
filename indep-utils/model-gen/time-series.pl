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
# An perl script that computes time-series of traffic size based the output
# of tcpdump trace in the following format <timstamp> <packet size>
# , can be further used wavelet scaling analysis of traffic
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066


$totalSize = 0;
$cur_time= 0;
$start=0;
$time_block = 0.001;

while (<STDIN>) {
      
    	($cur_time,$size) = split(' ',$_);

	if ($start eq 0) {
	  	$cur_epoch = $cur_time - 1;
	  	$start=1;
	}

	if ($cur_time > $cur_epoch) {
	    	while ($cur_epoch < $cur_time) {
	        	print "$totalSize\n";
			$cur_epoch = $cur_epoch + $time_block;
	        	$totalSize=0;
	    	}
	    	if ($cur_time < $cur_epoch) {$totalSize=$size;}
	} else {
	    	$totalSize=$totalSize + $size;
	}
}
	
