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
	usage: $0 [-s DomainPrefix] [-w FilenameExtention]
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


($ip1,$ip2,$ip3,$ip4,$m1,$m2,$m3,$m4) = split(/[.\/ ]/,$prefix);

$r1= 255 - $m1;
$r2= 255 - $m2;
$r3= 255 - $m3;
$r4= 255 - $m4;

$ip1h=$ip1+$r1;
$ip2h=$ip2+$r2;
$ip3h=$ip3+$r3;
$ip4h=$ip4+$r4;

if ($ip1h >  255) {
   $ip1=0;
   $ip1h=255;
}
if ($ip2h >  255) {
   $ip2=0;
   $ip2h=255;
}
if ($ip3h >  255) {
   $ip3=0;
   $ip3h=255;
}
if ($ip4h >  255) {
    $ip4=0;
    $ip4h=255;
}

$woutf=join(".",$fext,"outbound");
$winf=join(".",$fext,"inbound");

$http_port="80";
$http_port_alias="http";
#$HttpPrefixPort=join(".",$prefix,$http_port);

open(WOUT,"> $woutf") || die("cannot open $woutf\n");
open(WIN,"> $winf") || die("cannot open $winf\n");


while (<>) {
        ($time1,$time2,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);
#        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);


#        $dummy0="";
        $dummy1="";
        $dummy2="";
	$time1=0;
	$time2=0;

#	$prefixc=join(".",$ip11,$ip12);
#	$prefixs=join(".",$ip21,$ip22);
#	$prefixcp=join(".",$ip11,$ip12,$srcPort);
#	$prefixsp=join(".",$ip21,$ip22,$dstPort);

       	#seperate Inbound and Outbound WWW traffic of ISI
#	if (( $prefixcp eq $HttpPrefixPort) || ( $prefixsp eq $HttpPrefixPort))
        if ((((($ip11 <= $ip1h) && ($ip11 >= $ip1)) &&
              (($ip12 <= $ip2h) && ($ip12 >= $ip2)) &&
              (($ip13 <= $ip3h) && ($ip13 >= $ip3)) &&
              (($ip14 <= $ip4h) && ($ip14 >= $ip4))) && 
	                    (($srcPort eq $http_port) || ($srcPort eq $http_port_alias))) ||
            (((($ip21 <= $ip1h) && ($ip21 >= $ip1)) &&
              (($ip22 <= $ip2h) && ($ip22 >= $ip2)) &&
              (($ip23 <= $ip3h) && ($ip23 >= $ip3)) &&
              (($ip24 <= $ip4h) && ($ip24 >= $ip4))) && 
	                    (($dstPort eq $http_port) || ($dstPort eq $http_port_alias))) ) 
{
		print WOUT "$_";
	} else {
#		if ((($prefixc ne $prefix) && ($srcPort eq $http_port)) || 
#		    (($prefixs ne $prefix) && ($dstPort eq $http_port))) 
                if ((!((($ip11 <= $ip1h) && ($ip11 >= $ip1)) &&
                      (($ip12 <= $ip2h) && ($ip12 >= $ip2)) &&
                      (($ip13 <= $ip3h) && ($ip13 >= $ip3)) &&
                       (($ip14 <= $ip4h) && ($ip14 >= $ip4))) && 
	                    (($srcPort eq $http_port) || ($srcPort eq $http_port_alias))) ||
                    (!((($ip21 <= $ip1h) && ($ip21 >= $ip1)) &&
                       (($ip22 <= $ip2h) && ($ip22 >= $ip2)) &&
                       (($ip23 <= $ip3h) && ($ip23 >= $ip3)) &&
                       (($ip24 <= $ip4h) && ($ip24 >= $ip4))) && 
	                    (($dstPort eq $http_port) || ($dstPort eq $http_port_alias)))) 
		{
			print WIN "$_";
		}
	}
}
close(WOUT);
close(WIN);
