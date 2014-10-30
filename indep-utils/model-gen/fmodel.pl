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
	    -r string  ftp trace
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

$fc=0;

while (<>) {
        ($ip,$type,$dummy) = split(/[\n ]/,$_);

	$dummy="";

 	if ($type eq "F") {
			$ftp[$fc]=$ip;
			$handle=join("","FTP",$fc);
			$fh[$fc]=$handle;
			$fc++;

            #initialize/clear the output file
			$fext=join("-","FTP",$fc);
			$outfile=join(".",$infile,$fext);

#			open($handle,"> $outfile") || die("cannot open $outfile\n");
			open($handle,"> $fext") || die("cannot open $outfile\n");
        }
}


open(FIN,"< $infile") || die("cannot open $infile\n");


while (<FIN>) {
        ($time1,$time2,$dummy0,$ip11,$ip12,$ip13,$ip14,$srcPort,$dummy1,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy2) = split(/[.:() ]/,$_);

	$dummy0="";
	$dummy1="";
	$dummy2="";
	$srcPort="";
	$dstPort="";
	$time1="";
	$time2="";


        foreach $j (0 .. $#ftp) {
		

		$prefix=$ftp[$j];
	 	$handle=$fh[$j];

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

      		if (((($ip11 <= $ip1h) && ($ip11 >= $ip1)) &&
             	     (($ip12 <= $ip2h) && ($ip12 >= $ip2)) &&
	    	     (($ip13 <= $ip3h) && ($ip13 >= $ip3)) &&
	    	     (($ip14 <= $ip4h) && ($ip14 >= $ip4))) ||
      		    ((($ip21 <= $ip1h) && ($ip21 >= $ip1)) &&
             	     (($ip22 <= $ip2h) && ($ip22 >= $ip2)) &&
	    	     (($ip23 <= $ip3h) && ($ip23 >= $ip3)) &&
	    	     (($ip24 <= $ip4h) && ($ip24 >= $ip4)))) {

			print $handle "$_";
        	}
    	 }

}

close(FIN);

#compute CDF for each FTP model
foreach $j (0 .. $#ftp) {
        $k=$j+1;
#	$fext=join("-","FTP",$k);
#	$outfile=join(".",$infile,$fext);
	$outfile=join("-","FTP",$k);
	$outfile1=join(".",$outfile,"flow.sort");
	$outfile2=join(".",$outfile,"arrive");
	$outfile3=join(".",$outfile,"arrive.sort");
	$outfile4=join(".",$outfile,"file.inter");
	$outfile5=join(".",$outfile,"size");
	$outfile6=join(".",$outfile,"fileno");
	$outfile7=join(".",$outfile,"session");
	$outfile8=join(".",$outfile,"session.sort");
	$outfile9=join(".",$outfile,"session.inter");

        $ret=`cat $outfile | awk -f ftp.awk | sort > $outfile1`;
	$ret=`cat $outfile1 | ftp.pl -w $outfile`;
	$ret=`sort -o $outfile3 $outfile2`;
	$ret=`awk -f ftp.arrive.awk < $outfile3 > $outfile4`;
	$ret=`sort -o $outfile8 $outfile7`;
	$ret=`awk -f arrive2inter.awk < $outfile8 > $outfile9`;

	$ret=`dat2cdf -e 0 -i 0.001 -d 1 -t $outfile4`;
	$ret=`dat2cdf -e 0 -i 1000 -d 1000 -t $outfile5`;
	$ret=`dat2cdf -e 0 -i 1 -d 1 -t $outfile6`;
	$ret=`dat2cdf -e 0 -i 1 -d 1 -t $outfile9`;
}

