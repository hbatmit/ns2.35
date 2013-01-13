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

open(WCLIENT,"< webCLIENT");
open(WSERVER,"< webSERVER");
open(FCLIENT,"< ftpCLIENT");
open(FSERVER,"< ftpSERVER");
open(FNODE,"> node.tcl");
open(WCDF,"> web.server.cdf");
open(FCDF,"> ftp.server.cdf");

$cnt=0;
while (<FSERVER>) {
        ($server,$rest)= split(/[\n ]/,$_);
	$cnt++;
}

print FNODE "set num_ftp_server $cnt\n";


foreach $j (1 .. $cnt) {
	$k=$j-1;
	$m=1.0/$cnt*$j;
	print FCDF "$k $j $m\n"; 
}


$cnt=0;
while (<WSERVER>) {
        ($server,$rest)= split(/[\n ]/,$_);
	$cnt++;
}

print FNODE "set num_web_server $cnt\n";


foreach $j (1 .. $cnt) {
	$k=$j-1;
	$m=1.0/$cnt*$j;
	print WCDF "$k $j $m\n"; 
}


$cnt=0;
while (<WCLIENT>) {
        ($client,$rest)= split(/[\n ]/,$_);
        $wclient[$cnt]=$client;
	$cnt++;
}

$wc=$cnt;


$common=0;
$cnt=0;

while (<FCLIENT>) {
        ($client,$rest)= split(/[\n ]/,$_);
	$c=1;
	foreach $j (0 .. $#wclient) {
      		if (($client eq $wclient[$j]) && ($c==1)) {
			$common++;
			$c=0;
		}
	}
	$cnt++;
}
$wc=$wc-$common;
$wf=$cnt-$common;


print FNODE "set num_common_client $common\n";
print FNODE "set num_web_client $wc\n";
print FNODE "set num_ftp_client $wf\n";



close(FCLIENT);
close(FSERVER);
close(WCLIENT);
close(WSERVER);
close(FNODE);
close(WCDF);
close(FCDF);

