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

open(FSERVER,"< ftpSERVER");
open(FNODE,"> fnode.tcl");
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




close(FSERVER);
close(FNODE);
close(FCDF);

