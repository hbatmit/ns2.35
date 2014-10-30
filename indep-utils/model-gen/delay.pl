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

sub usage {
        print STDERR <<END;
	usage: $0 [-p Port] 
	Options:
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
my($dbopts) = new DbGetopt("p:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 'p') {
                $port = $dbopts->optarg;
        } else {
                &usage;
        };
};


while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy,$ip21,$ip22,$ip23,$ip24,$dstPort,$flag,$seq1,$win,$seq2,$others) = split(/[. ]/,$_);
#        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy,$ip21,$ip22,$ip23,$ip24,$dstPort,$flag,$seq1,$win,$seq2,$others) = split(/[. ]/,$_);


#        $dummy0="";
        $dummy="";
	$others="";

        $time=join(".",$time1,$time2);
	$src=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);

	$client=$src;
	$server=$dst;

        if ($srcPort eq $port) {
		$client=$dst;
		$server=$src;
	}

        if ($flag eq "S") {
	   print "$client $server $time $seq1 $win $seq2\n";
	}
}
