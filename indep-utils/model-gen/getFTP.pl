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
	usage: $0 [-r Filename]
	Options:
	    -r string  file that contains a list of FTP clients, which
	               is output of getFTPclient.pl
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
my($dbopts) = new DbGetopt("r:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
    	$ch = $dbopts->opt;
        if ($ch eq 'r') {
		$infile = $dbopts->optarg;
    	} else {
            	&usage;
        };
};


open(FIN,"< $infile") || die("cannot open $infile\n");

$special_port=1024;
$ftp_port=20;
$num_ftp=0;

while (<FIN>) {
   ($host,$newline) = split(/[:() \n]/,$_);
   $newline="";
   $ftpC[$num_ftp]=$host;
   $num_ftp++;
}

close(FIN);


while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$flag,$dummy2) = split(/[.:() ]/,$_);
#        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$flag,$dummy2) = split(/[.:() ]/,$_);


#        $dummy0="";
        $dummy1="";
        $dummy2="";

	$time1=0;
	$time2=0;

	$src=join(".",$ip11,$ip12,$ip13,$ip14);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24);


        if ((($srcPort > $special_port) && ($dstPort > $special_port)) ||
            (($srcPort > $special_port) && ($dstPort == $ftp_port)) ||
            (($srcPort == $ftp_port) && ($dstPort > $special_port))) {


	           $found=0;
		   foreach $j (0 .. $#ftpC) {
		    	if (($src eq $ftpC[$j]) || ($dst eq $ftpC[$j])) {
		          	$found=1;
                   	}
		   }

		   if (($found eq 1)  && ($flag ne "udp")) {
		   	print "$_";
		   }
	}
}
