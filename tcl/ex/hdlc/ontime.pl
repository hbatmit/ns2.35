#!/usr/bin/perl


# get arguments ontime.pl <duration> <filename> <#i> <retries>

$simtime = shift;  # length of simulation run
$filename = shift; # file to write into
$i = shift;        # run #
$retries = shift;  # number of retries (0,1,..5)

print "$i, $simtime, $filename $retries\n";

$linkrate = 10000000;
$pktsize = 1040;  #TCP
#$pktsize = 512; #CBR





#get the totalpackets and numrexmit'ed packets number
# this method is inaccurate compared to getting pkt num from ns trace outfile
#$ndatapack = 0;
#$nrexmitpack = 0;

# for tracing seqno and cwnd values
open(SEQ, ">seqno.$filename.$i") || die "Can't open seqno.$filename.$retries.$i\n";
#open(CWND, ">cwnd.$filename.$i") || die "Can't open seqno.$filename.$retries.$i\n";

#open file for writing/appending
open(LOG1, ">>$filename") || die "Can't open $filename\n";
open(LOG2, ">>$filename.stat") || die "Can't open $filename.stat\n";

open(NSTRACE, "case0.tcp_trace.$i") || die "Can't open case0.tcp_trace.$i\n";
while ($temp = <NSTRACE>) {
    chomp($temp);
    @line = split " ", $temp;
    $var = @line[5];
    $val = @line[6];
    #if ($var eq "ndatapack_") {
	#$ndatapack = $val;
    #}
    #elsif ($var eq "nrexmitpack_") {
	#$nrexmitpack = $val;
    #}
    if ($var eq "t_seqno_") {
	print SEQ "@line[0] $val\n";
    }
    #elsif ($var eq "cwnd_") {
	#print CWND "@line[0] $val\n";
    #}
}	
#print ("ndatapack= $ndatapack, nrexmitpack = $nrexmitpack\n");
#$pktrecvd = $ndatapack - $nrexmitpack;
#print ("pktrecvd = $pktrecvd\n");
#$datarecvd = $pktrecvd * $pktsize ;  # in bytes
#print("datarecvd = $datarecvd\n");

# find total num of pkts recvd at sink from tracefile
# this doesn't work as the tracefile doesn't seem to be tracing recvs at the agent layer (TCP)
$bytenum = 0;
open(TRFILE, "case0.tr") || die "Can't open case0.tr\n";
while ($temp = <TRFILE>) {
    chomp($temp);
    @line = split " ", $temp;
    $op = @line[0];
    $s = @line[2];
    $d = @line[3];
    if ($op eq "r") {
	if ($s == 0 && $d == 1 ) {
	    $p = @line[4];	
	    if (($p eq "tcp") || ($p eq "cbr")) {
	    #if ($s == 0 && $d == 1 ) {
		$tcppkts = @line[10];
	    }	
	    #if (($op eq "r") || ($op eq "e")) {
	    #if ($s == 0 && $d == 1 ) {
	    $bytenum = $bytenum + @line[5];
	}
    }	
}

print "total tcp pkts recvd = $tcppkts\n";
$datarecvd = $tcppkts * $pktsize ;  # in bytes
print("total tcp bytes recvd = $datarecvd\n");
print "total link bytes recvd = $bytenum\n";

# calculate on and off times from timer state output
# first get the ON/OFF timestamps from tracefile

#for ontime distribution
$a1 = 0;  # less than 1
$a2 = 0;
$a3 = 0;
$a4 = 0;  # between 1 and 5
$a5 = 0;  # between 5 and 10
$a6 = 0;  # between 10 and 50
$a7 = 0;  # more than 50
$a8 = 0;
$a9 = 0;
$a10 = 0;  # less than 1
$a11 = 0;
$a12 = 0;
$a13 = 0;  # between 1 and 5
$a14 = 0;  # between 5 and 10
$a15 = 0;  # between 10 and 50
$a16 = 0;  # more than 50
$a17 = 0;
$a18 = 0;
$a19 = 0;
$a20 = 0;
$a21 = 0;
$a22 = 0;
$a23 = 0;
$a24 = 0;
$a25 = 0;
$a26 = 0;

#for offtime distribution
$p1 = 0;  # less than 1
$p2 = 0;
$p3 = 0;
$p4 = 0;  # between 1 and 5
$p5 = 0;  # between 5 and 10
$p6 = 0;  # between 10 and 50
$p7 = 0;  # more than 50
$p8 = 0;
$p9 = 0;
$p10 = 0;  # less than 1
$p11 = 0;
$p12 = 0;
$p13 = 0;  # between 1 and 5
$p14 = 0;  # between 5 and 10
$p15 = 0;  # between 10 and 50
$p16 = 0;  # more than 50
$p17 = 0;
$p18 = 0;  # between 1 and 5
$p19 = 0;  # between 5 and 10
$p20 = 0;  # between 10 and 50
$p21 = 0;

open(STATES, "+< states.tr") || die "Can't open states.tr\n";

$offtime = 0;
$ontime = 0;

$temp = <STATES>;
chomp($temp);
@line = split " ", $temp;
$t1 = @line[1];		
$d1 = @line[7];		
$timer1 = @line[3];	
$max = 0;
$num = 0;

if ($d1 > $max) {
    $max = $d1;
}

while ($temp = <STATES>) {
    chomp($temp);
    @line = split " ", $temp;
    $t2 = @line[1];
    $d2 = @line[7];
    $timer2 = @line[3];
    
    if ($d2 > $max) {
	$max = $d2;
    }
    
    $old_ontime = $ontime;
    $old_offtime = $offtime;
    #print "old_ontime = $ontime; old_offtime = $offtime\n";
    #compare_OR_onoff_times($t1, $d1, $timer1, $t2, $d2, $timer2, $ontime, $offtime);

    # state is on if either timer1 or timer2 is on
    if ($timer1 eq $timer2) { #same timer
	$ontime += $d1;
	$offtime += $t2 - ($t1 + $d1);
	$t1 = $t2;
	$d1 = $d2;
	$timer1 = $timer2;
	$num++;
    }	 
    else { # different timer
	if ($t2 >= ($t1 + $d1)) {
	    $ontime += $d1;
	    $offtime += $t2 - ($t1 + $d1);
	    $t1 = $t2;	
	    $d1 = $d2;	
	    $timer1 = $timer2;
	    $num++;
	}
	else { # t2 < ($t1+$d1)
	    if ( $t1+$d1 >= $t2+$d2 ) {
		
	    } 
	    else { # t1+d1 < t2+d2
		$ontime += $t2 - $t1;
		$t1 = $t2;	
		$d1 = $d2;	
		$timer1 = $timer2;
		$num++;

	    }
	}
    }

    #print "ontime = $ontime; offtime = $offtime\n";
    $delta_ontime = $ontime - $old_ontime;
    $delta_offtime = $offtime - $old_offtime;


    # creating distributions for on and off time
    if ($delta_ontime > 0) {
	if ($delta_ontime < 2) {
	    $a1++;
	} elsif ($delta_ontime >= 2 && $delta_ontime < 4) {
	    $a2++;
	} elsif ($delta_ontime >= 4 && $delta_ontime < 6) {
	    $a3++;
	} elsif ($delta_ontime >= 6 && $delta_ontime < 8) {
	    $a4++;
	} elsif ($delta_ontime >= 8 && $delta_ontime < 10) {
	    $a5++;
	} elsif ($delta_ontime >= 10 && $delta_ontime < 12) {
	    $a6++;
	} elsif ( $delta_ontime >= 12 && $delta_ontime < 14) {
	    $a7++;
	} elsif ($delta_ontime >= 14 && $delta_ontime < 16) {
	    $a8++;
	} elsif ( $delta_ontime >= 16 && $delta_ontime < 18) {
	    $a9++;
	} elsif ($delta_ontime >= 18 && $delta_ontime < 20) {
	    $a10++;
	} elsif ($delta_ontime >= 20 && $delta_ontime < 22) {
	    $a11++;
	} elsif ($delta_ontime >= 22 && $delta_ontime < 24) {
	    $a12++;
	} elsif ($delta_ontime >= 24 && $delta_ontime < 26) {
	    $a13++;
	} elsif ($delta_ontime >= 26 && $delta_ontime < 28) {
	    $a14++;
	} elsif ($delta_ontime >= 28 && $delta_ontime < 30) {
	    $a15++;
	} elsif ( $delta_ontime >= 30 && $delta_ontime < 32) {
	    $a16++;
	} elsif ($delta_ontime >= 32 && $delta_ontime < 34) {
	    $a17++;
	} elsif ( $delta_ontime >= 34 && $delta_ontime < 36) {
	    $a18++;
	} elsif ($delta_ontime >= 36 && $delta_ontime < 38) {
	    $a19++;    
	} elsif ( $delta_ontime >= 38 && $delta_ontime < 40) {
	    $a20++;
	} elsif ($delta_ontime >= 40 && $delta_ontime < 42) {
	    $a21++;
	} elsif ( $delta_ontime >= 42 && $delta_ontime < 44) {
	    $a22++;
	} elsif ($delta_ontime >= 44 && $delta_ontime < 46) {
	    $a23++;
	} elsif ($delta_ontime >= 46 && $delta_ontime < 48) {
	    $a24++;
	} elsif ($delta_ontime >= 48 && $delta_ontime < 50) {
	    $a25++;    
	} elsif ($delta_ontime >= 50) {
	    $a26++;
	}
    }	
    
    #print "delta_ontime=$delta_ontime a1=$a1 a2=$a2 a3=$a3 b=$b c=$c d=$d e=$e\n";

    #now for distribution for offtimes
    if ($delta_offtime > 0) {
	if ($delta_ontime < 0.2) {
	    $p1++;
	} elsif ($delta_ontime >= 0.2 && $delta_ontime < 0.4) {
	    $p2++;
	} elsif ($delta_ontime >= 0.4 && $delta_ontime < 0.6) {
	    $p3++;
	} elsif ($delta_ontime >= 0.6 && $delta_ontime < 0.8) {
	    $p4++;
	} elsif ($delta_ontime >= 0.8 && $delta_ontime < 1.0) {
	    $p5++;
	} elsif ($delta_ontime >= 1.0 && $delta_ontime < 1.2) {
	    $p6++;
	} elsif ( $delta_ontime >= 1.2 && $delta_ontime < 1.4) {
	    $p7++;
	} elsif ($delta_ontime >= 1.4 && $delta_ontime < 1.6) {
	    $p8++;
	} elsif ( $delta_ontime >= 1.6 && $delta_ontime < 1.8) {
	    $p9++;
	} elsif ($delta_ontime >= 1.8 && $delta_ontime < 2.0) {
	    $p10++;
	} elsif ($delta_ontime >= 2.0 && $delta_ontime < 2.2) {
	    $p11++;
	} elsif ($delta_ontime >= 2.2 && $delta_ontime < 2.4) {
	    $p12++;
	} elsif ($delta_ontime >= 2.4 && $delta_ontime < 2.6) {
	    $p13++;
	} elsif ($delta_ontime >= 2.6 && $delta_ontime < 2.8) {
	    $p14++;
	} elsif ($delta_ontime >= 2.8 && $delta_ontime < 3.0) {
	    $p15++;
        } elsif ( $delta_ontime >= 3.0 && $delta_ontime < 3.2) {
	    $p16++;
	} elsif ($delta_ontime >= 3.2 && $delta_ontime < 3.4) {
	    $p17++;
	} elsif ( $delta_ontime >= 3.4 && $delta_ontime < 3.6) {
	    $p18++;
	} elsif ($delta_ontime >= 3.6 && $delta_ontime < 3.8) {
	    $p19++;    
	} elsif ( $delta_ontime >= 3.8 && $delta_ontime < 4.0) {
	    $p20++;
        } elsif ($delta_ontime >= 4.0) {
	    $p21++;
	}
    }

    #print "delta_offtime=$delta_offtime p1=$p1 p2=$p2 p3=$p3 q=$q r=$r s=$s t=$t\n";
    
}

# check for end of sim cases
if (($t1+$d1) > $simtime) {
    $ontime += $simtime - $t1;
} else {
    $ontime += $d1;
}	
$num++;

# creating distributions for on and off time
if ($delta_ontime < 2) {
    $a1++;
} elsif ($delta_ontime >= 2 && $delta_ontime < 4) {
    $a2++;
} elsif ($delta_ontime >= 4 && $delta_ontime < 6) {
    $a3++;
} elsif ($delta_ontime >= 6 && $delta_ontime < 8) {
    $a4++;
} elsif ($delta_ontime >= 8 && $delta_ontime < 10) {
    $a5++;
} elsif ($delta_ontime >= 10 && $delta_ontime < 12) {
    $a6++;
} elsif ( $delta_ontime >= 12 && $delta_ontime < 14) {
    $a7++;
} elsif ($delta_ontime >= 14 && $delta_ontime < 16) {
    $a8++;
} elsif ( $delta_ontime >= 16 && $delta_ontime < 18) {
    $a9++;
} elsif ($delta_ontime >= 18 && $delta_ontime < 20) {
    $a10++;
} elsif ($delta_ontime >= 20 && $delta_ontime < 22) {
    $a11++;
} elsif ($delta_ontime >= 22 && $delta_ontime < 24) {
    $a12++;
} elsif ($delta_ontime >= 24 && $delta_ontime < 26) {
    $a13++;
} elsif ($delta_ontime >= 26 && $delta_ontime < 28) {
    $a14++;
} elsif ($delta_ontime >= 28 && $delta_ontime < 30) {
    $a15++;
} elsif ( $delta_ontime >= 30 && $delta_ontime < 32) {
    $a16++;
} elsif ($delta_ontime >= 32 && $delta_ontime < 34) {
    $a17++;
} elsif ( $delta_ontime >= 34 && $delta_ontime < 36) {
    $a18++;
} elsif ($delta_ontime >= 36 && $delta_ontime < 38) {
    $a19++;    
} elsif ( $delta_ontime >= 38 && $delta_ontime < 40) {
    $a20++;
} elsif ($delta_ontime >= 40 && $delta_ontime < 42) {
    $a21++;
} elsif ( $delta_ontime >= 42 && $delta_ontime < 44) {
    $a22++;
} elsif ($delta_ontime >= 44 && $delta_ontime < 46) {
    $a23++;
} elsif ($delta_ontime >= 46 && $delta_ontime < 48) {
    $a24++;
} elsif ($delta_ontime >= 48 && $delta_ontime < 50) {
    $a25++;    
} elsif ($delta_ontime >= 50) {
    $a26++;
}	

	
close(STATES);
print LOG2 "$a1 $a2 $a3 $a4 $a5 $a6 $a7 $a8 $a9 $a10 $a11 $a12 $a13 $a14 $a15 $a16 $a17 $a18 $a19 $a20 $a21 $a22 $a23 $a24 $a25 $a26 $p1 $p2 $p3 $p4 $p5 $p6 $p7 $p8 $p9 $p10 $p11 $p12 $p13 $p14 $p15 $p16 $p17 $p18 $p19 $p20 $p21 $num\n";
close(LOG2);

print("Ontime: $ontime\n");
print ("Offtime: $offtime\n");

#calculate the goodput

$tcpgoodput1 = ($datarecvd * 8) / $simtime ;
$tcpgoodput2 = ($datarecvd * 8) / $ontime ;

$linkthruput1 = ($bytenum * 8) / $simtime;
$linkthruput2 = ($bytenum * 8) / $ontime;

$idealput = $linkrate * ($ontime/$simtime);

#print LOG "Run$i(in bps)
#\tOntime: $ontime Offtime: $offtime Linkrate(bps): $linkrate
#\tTotalPktsRecvd: $pktrecvd TotalBytesRecvd: $datarecvd 
#\tGoodput1: (total_bytes*8)/simtime = $goodput1 (in bps)
#\tGoodput2: (total_bytes*8)/ONtime = $goodput2 (in bps)
#\tidealput: linkrate * ONtime/simtime = $idealput (in bps)\n\n";

printf LOG1 "%.4f %.4f %.4f %.4f %.4f %.4f\n", $ontime,$tcpgoodput1,$tcpgoodput2,$linkthruput1,$linkthruput2,$idealput;

close(LOG1);

sub put_in_bin () {
    $dur2 = shift;
    $a1 = shift;
    $a2 = shift;
    $a3 = shift;
    $b = shift;
    $c = shift;
    $d = shift;
    $e = shift;
    
    if ($dur2 < 0.25) {
	$a1++;
    } elsif ($dur2 >= 0.25 && $dur2 < 0.5) {
	$a2++;
    } elsif ($dur2 >= 0.5 && $dur2 < 1) {
	$a3++;
    } elsif ( $dur2 >= 1 && $dur2 < 5) {
	$b++;
    } elsif ($dur2 >= 5 && $dur2 < 10) {
	$c++;
    } elsif ($dur2 >= 10 && $dur2 < 50) {
	$d++;
    } elsif ($dur2 >=50) {
	$e++;
    }
}	

# state is ON when either timer1 or timer2 is ON
sub compare_OR_onoff_times () {
    $t1 = shift;
    $d1 = shift;
    $timer1 = shift;
    $t2 = shift;
    $d2 = shift;
    $timer2 = shift;
    $ontime = shift;
    $offtime = shift;
    
    if ($timer1 eq $timer2) { #same timer
	$ontime += $d1;
	$offtime += $t2 - ($t1 + $d1);
	$t1 = $t2;
	$d1 = $d2;
	$timer1 = $timer2;
	print "type1\n";
	return;
    } 
    else { # different timer
	if ($t2 >= ($t1 + $d1)) {
	    $ontime += $d1;
	    $offtime += $t2 - ($t1 + $d1);
	    $t1 = $t2;	
	    $d1 = $d2;	
	    $timer1 = $timer2;
	    print "type2\n";
	    return;
	}
	else { # t2 < ($t1+$d1)
	    if ( $t1+$d1 >= $t2+$d2 ) {
		print "type3\n";
		return;
	    } 
	    else { # t1+d1 < t2+d2
		$ontime += $t2 - $t1;
		$t1 = $t2;	
		$d1 = $d2;	
		$timer1 = $timer2;
		print "type4\n";
		return;
	    }
	}
    }
}

# state is on only when both timer1 and timer2 is ON
sub compare_AND_onoff_times {
    
#calculate the ontime by first calculating the offtime

    # same timer
    if ($timer1 eq $timer2) {
	#$offtime += $dur1;
	$start1 = $start2;
	$dur1 = $dur2;
	$end1 = $end2;
	$timer1 = $timer2;
	print("Case1: Offtime = $offtime\n");
    }
    else {	
	#if ($end1 > $end2) {	
	#   continue;
	#}		
	if ($end1 <= $start2) {
	    $offtime += $dur1;
	    $start1 = $start2;
	    $dur1 = $dur2;
	    $end1 = $end2;
	    $timer1 = $timer2;
	    print("Case2: Offtime = $offtime\n");
	}		
	elsif (($end1 > $start2) && ($end1 <= $end2)) {
	    $offtime += $start2 - $start1;
	    $start1 = $start2;
	    $dur1 = $dur2;
	    $end1 = $end2;
	    $timer1 = $timer2;
	    print("Case3: Offtime = $offtime\n");
	}
	else {
	    # $end1 > $end2
	    $flag = 1;	
 	}
    }	
    print("Offtime = $offtime\n");
}
if ($flag == 1) {
    if ($end1 > $simtime) {
	$offtime += $simtime - $start1;
    } else {
	$offtime += $dur1;	
    }
 }     
else {
    # we have to add the remaining off duration now
    if (($start2 + $dur2) > $simtime) {
	$offtime += $simtime - $start2;
    } else {
	$offtime += $dur2;
    }	
}
