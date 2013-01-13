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
      usage: $0  [-w FilenameExtention]
        Options:
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
my($dbopts) = new DbGetopt("w:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 'w') {
                $fext = $dbopts->optarg;
	} else {
                &usage;
        };
};

$fsession=join(".",$fext,"session");
$farrivef=join(".",$fext,"arrive");
$sizef=join(".",$fext,"size");
$filef=join(".",$fext,"fileno");

open(FSESSION,"> $fsession") || die("cannot open $fsession\n");
open(FFARRIVE,"> $farrivef") || die("cannot open $farrivef\n");
open(FSIZE,"> $sizef") || die("cannot open $sizef\n");
open(FFILE,"> $filef") || die("cannot open $filef\n");


$fileno=0;
$fsize=0;
$oldsrc="";
$olddst="";
$oldsrcP="";
$olddstP="";


while (<>) {
        ($ip11,$ip12,$ip13,$ip14,$srcPort,$ip21,$ip22,$ip23,$ip24,$dstPort,$dummy,$time1,$time2,$flag,$seqb,$seqe,$size,$size1) = split(/[.:() ]/,$_);


        $time=join(".",$time1,$time2);

        if (!defined($size)) {
	        $size=0;
	} else {
		if (($flag eq "") && ($seqb eq "") && defined($size1)) {     
			$size=$size1;
		}
	}
        if (!defined($flag)) {
	        $flag="";
	}

        $dummy="";
	$seqe="";

        #not modelig FTP control channel
        if (($srcPort ne "21") && ($dstPort ne "21")) {

        $srcP=join(".",$ip11,$ip12,$ip13,$ip14,$srcPort);
	$dstP=join(".",$ip21,$ip22,$ip23,$ip24,$dstPort);
        $src=join(".",$ip11,$ip12,$ip13,$ip14);
	$dst=join(".",$ip21,$ip22,$ip23,$ip24);
        
        if ($flag eq "S") {
		$ssrcP=$srcP;
		$sdstP=$dstP;
		$ssrc=$src;
		$sdst=$dst;
		$stime=$time;
	}

	if (($src eq $oldsrc) && ($dst eq $olddst)) {
		if (($srcP eq $oldsrcP) && ($dstP eq $olddstP)) {
		        $fsize=$fsize+$size;
        		if ((($flag eq "F") || ($flag eq "FP")) && 
		    		($ssrcP eq $srcP) && ($sdstP eq $dstP)) {
		    		$fileno++;
				if ($fsize != 0) {
#					print FSIZE "$srcP $dstP $fsize\n";
					print FSIZE "$fsize\n";
				}
				$fsize=0;
                                print FFARRIVE "$ssrc $sdst $stime\n";
			}
		}
	} else {
	        if (($oldsrc ne "") && ($olddst ne "")) {
			if ($fileno != 0) {
#				print FFILE "$oldsrc $olddst $fileno\n";
                                print FSESSION "$oldtime\n";
				print FFILE "$fileno\n";
			}
		}
		$fileno=0;
		$oldtime=$time;
	}

	$oldsrc=$src;
	$olddst=$dst;
	$oldsrcP=$srcP;
	$olddstP=$dstP;
	}
}

#print the last FTP session
print FFILE "$fileno\n";


close(FFARRIVE);
close(FSIZE);
close(FFILE);
