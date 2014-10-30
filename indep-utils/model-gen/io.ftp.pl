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
	usage: $0 [a] [-s DomainPrefix] [-p Port] [-w FilenameExtention]
	Options:
	    -s string  specify IP prefix to distinguish Inbound from outbound
	               traffic (eg. 192.1)
	    -w string  specify the filename extention

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
my($dbopts) = new DbGetopt("s:w:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
    	$ch = $dbopts->opt;
        if ($ch eq 's') {
		$prefix = $dbopts->optarg;
	} elsif ($ch eq 'w') {
	        $fext = $dbopts->optarg;
    	} else {
            	&usage;
        };
};


$foutf=join(".",$fext,"outbound");
$finf=join(".",$fext,"inbound");

$ftp_port="21";
$ftpdata_port="20";

open(FOUT,"> $foutf") || die("cannot open $foutf\n");
open(FIN,"> $finf") || die("cannot open $finf\n");


while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);


#        $dummy0="";
        $dummy1="";
        $dummy2="";

	$time1=0;
	$time2=0;
	$ip13="";
	$ip14="";
	$ip23="";
	$ip24="";

	$prefixc=join(".",$ip11,$ip12);
	$prefixs=join(".",$ip21,$ip22);

       	#seperate Inbound and Outbound FTP traffic of ISI
	if ((( $prefixc eq $prefix) && (( $srcPort eq $ftp_port) || ( $srcPort eq $ftpdata_port))) || (( $prefixs eq $prefix) && (($dstPort eq $ftp_port) || ($dstPort eq $ftpdata_port)))) {
		print FOUT "$_";
	} else {
		print FIN "$_";
	}
}
close(FOUT);
close(FIN);
