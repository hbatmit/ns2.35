#!/usr/bin/perl

$stem = shift;	# the base name of the trace files to use, right now its 'high-speed'
print "$stem";
$source = shift || 0;
$numsources = $source;
$source = "$source.0" unless $source =~ /\./;
($type = $stem ) =~ s/^(.)/\U$1\u/; # hackery to label graphs

#number of cross-traffic sources
$numcsources = shift;
print "numcsrouces = $numcsources";
# protocol being used
$tcp_type = shift;

# These are the temporary files that will be generated to be read by gnuplot.
# These files can all be deleted at the end of the script.
# To delete use the unlink command at the end.
$queue_data = "qd";
$drop_data = "drop";

$cwnd_data = "cwnd";
$seqno_data = "seqno";

$fairness_data = "fairness";
$fair_alloc = "fairalloc";

$util_data = "util";
# Used to plot the link utilization graph
$time_int = 1.0000;
$bandwidth = 10000000;
$pktsize = 1040;

#for gnuplot
$graphtype = "($numsources $tcp_type  sources, with $numcsources crosstraffic flows )";

# Open the queue trace for reading and traces for writing queue length,
# sequence number dequeued and dropped seqnos.  N. B. any of these failing will
# terminate the script with an error message.

#open(QT, "$stem.queue1") || die "Can't open $stem.queue1: $!\n";
#open(QD, ">$queue_data")  || die "can't open $queue_data :$!\n";
#open(DD, ">$drop_data")  || die "can't open $drop_data :$!\n";
#open(UT, ">$util_data") || die "cant open $util_data";

# parse the trace file.  The regexp comments the names of the fields, which are
# described in the everything.ps document.
#while (<QT>) {
#    chomp;
#    /(.)\s+		# operation
#     ([\\.\d]+)\s+	# time
#     (\d+)\s+		# from node
#     (\d+)\s+		# to node
#     (\w+)\s+		# protocol
#     (\d+)\s+		# size
#     ([\w+\-]+)\s+	# flags
#     (\d+)\s+		# flow id
#     (\d+\.\d+)\s+	# from addr (src.port)
#     (\d+\.\d+)\s+	# to addr (src.port)
#     (\d+)\s+		# seqno
#     (\d+)		# ns id
#     /x && do {
#	($op, $t, $n1, $n2, $p, $sz, $fl, $fid, $src, $dst, $seqno, $nsid) =
#	    ($1, $2, $3, $4, $5, $6 ,$7, $8, $9, $10, $11, $12);

	# this keeps a running queue length in $q.  That length is output to
	# the $queue_data file on every dequeue or drop - note that this may
	# show drops occurring before undropped packets are dequeued.
	#
	# If the dequeue/drop is from source 0, a record is output to either
	# the sequence number or drop file.
#	if ($op eq '+' ) { $q ++; }
#	elsif ($op eq '-' ) { 
#	    $q--;
#	    print QD "$t $q\n"; 
#	    #print SD "$t $seqno\n" if $src eq $source;
#	}
#	elsif ($op eq 'd' ) { 
#	    $q--; 
#	    print QD "$t $q\n";
#	    #print(DD "$t $seqno\n") && $sawdrop++ if $src eq $source;
#	    print (DD "$t $seqno\n") && $sawdrop++;
#	}

#checks for packets send out. Used to calculate link utilization.

#	if ($op eq '-') { 
#		if($t <= $time_int) {
#			$packets_seen++;
#		}
#		else {
#			$temp = ($packets_seen) / ( ($bandwidth/(8*$pktsize)) );
#			$temp = $temp * 100;
#			print UT "$time_int $temp\n";
#			$temp = 0;
#			$packets_seen = 1;
#			$time_int = $time_int + 1.0;
#		}
#	    }
#	next;
#    };
    # if the trace is corrupt or there's a valid value that the regexp doesn't
    # catch, this error message will come out.
#    printf STDERR "unmatched in line $.: $_\n";
#}

# put a dummy drop out if no drops have been seen or gnuplot will fail.
#print DD "0 0\n" unless $sawdrop;
# Close up these files.
#close(QL);
#close(QD);
#close(DD);
#close(UT);

# CWND processing.  Same ideas, different file.
print "Calculating cwnd and seqno for each source\n";
for ($i=1; $i <= $numsources ; $i++) {

	printf "Opening: $stem.tcp_trace.$i\n";
	open(CWND, "$stem.tcp_trace.$i") || die "Can't open $stem.tcp_trace.$i: $!\n";
	printf "Opening: $cwnd_data$i\n";
	open(CD, ">$cwnd_data$i")  || die "can't open $cwnd_data$i :$!\n";
	printf "Opening: $seqno_data$i\n";
	open(SD, ">$seqno_data$i")  || die "can't open $seqno_data$i :$!\n";

	# This loop is more straightforward.  I ignore everything but the time and cwnd
	# and print them.

	while (<CWND>) {
    		chomp;
    		/(\d*\.\d+)\s+  # time
     		(\d+)\s+		# dontcare
     		(\d+)\s+		# dontcare
     		(\d+)\s+		# dontcare
     		(\d+)\s+		# dontcare
     		(\w+)\s+		# variable we are tracing (cwnd or seqno)
     		(\d+)		# value
     		/x && do {
			($t, $n1, $n2, $p, $sz, $var, $value) =
		    ($1, $2, $3, $4, $5, $6, $7);

			if ($var eq 'cwnd_' ) {
			    print CD "$t $value\n";
			}
			
			if ($var eq 't_seqno_' ) {
			    print SD "$t $value\n";
			}
    		}
	}
	close(CWND);
	close(SD);
	close(CD);
}

##################### THIS IS FOR PLOTTING FAIRNESS GRAPH

# This says after what time should the calculation for Fairness should start
$startTime = shift;
print "starttime = $startTime \n";

# All seqno info is stored in files starting with seqno
$seqno_data = "seqno";

# This will have the number of packets send out in a perticular interval 
# by a perticular flow
$seqnoStart[0]=0;
$seqnoEnd[0]=0;
$noPackets[0]=0;
$eachShare[0]=0;
open(FR,">fairness");
open(FA,">fairalloc");

use IO::File;

$i=1;
while($i <= $numsources) {
	$filehandle[$i] = IO::File->new("$seqno_data$i") || die "cannot open";
	$i += 1;
}

# this for loop brings all file pointers to the same start position
# with respect to time
for($tmp=1; $tmp <= $numsources ; $tmp++) {
	$line = $filehandle[$tmp]->getline();
	chomp($line);
	($t, $seqno) = split(/ /,$line);
	while($t<$startTime) {
		$line = $filehandle[$tmp]->getline();
		chomp($line);
		($t, $seqno) = split(/ /, $line);
	}
	print "$t:$seqno:\n";
}

for( $j = $startTime ; $exitFlag < 1 ; $j++){
	for( $k=1 ; $k <= $numsources ; $k++) {
		$noPackets[$k]=0;
		$line = $filehandle[$k]->getline();
		$noPackets[$k]++;
		chomp($line);
		($t, $seqno) = split(/ /, $line);
		$seqnoStart[$k]=$seqno;
		while($t < ($j+1)){
			$line = $filehandle[$k]->getline();
			if ($line eq "") {
				if ($k == $numsources) {
					$exitFlag=1;
					last;
				}
				else {
					last;
				}
			}
			$noPackets[$k]++;
			chomp($line);
			($t, $seqno) = split(/ /, $line);
			if($t >= ($j+1)) {
				$seqnoEnd[$k]=$seqno;
				last;
			}
		}
		print "startseqno = $seqnoStart[$k] endSeqno = $seqnoEnd[$k] packets= $noPackets[$k]\n";
	}
	$totalPackets=0;
	for( $l=1 ; $l <= $numsources; $l++){
		$totalPackets += $noPackets[$l];
		open(FAa, ">>$fair_alloc$l")  || die "can't open $fair_alloc$i :$!\n";
		print FAa "$j $noPackets[$l]\n"; 
		close(FAa);
	}
	print "totalpackets for $j = $totalPackets \n";
	$fairallocation = $totalPackets / $numsources;
	print FA "$j $fairallocation\n";
	$denominator = 0;
	$numerator =0;

	for($l=1 ; $l <=$numsources; $l++) {
		$eachShare[$l] = ($noPackets[$l] / $fairallocation);
		print "each share $l = $eachShare[$l] \n";
		$denominator += ($eachShare[$l] ** 2);
		$numerator += $eachShare[$l];
	}
	$numerator = $numerator ** 2;
	$denominator = $denominator * $numsources;
	$fair = $numerator / $denominator;
	print FR "$j $fair\n";
	print "Fairness = $j $fair\n\n";

}
close(FR);
close(FA);

####################### End Fairness Plot

# Generate required strings for gnuplot. This shows my illetracy in Perl and gnuplot :(

$numsourcesplusone = $numsources + 1;
for($i=1; $i<=$numsources ; $i++){
	$plotstring_seqno = $plotstring_seqno . " \"$seqno_data$i\" title \"flow$i\" with points $i";
	if ($i != $numsources){
		$plotstring_seqno = $plotstring_seqno . "\,";
	}
}
$plotstring_seqno = $plotstring_seqno . "\,  \"$drop_data\" title \"dropped\" with points $numsourcesplusone";
print $plotstring_seqno;

for($i=1; $i<=$numsources ; $i++){
	$plotstring_cwnd = $plotstring_cwnd . " \"$cwnd_data$i\" title \"flow$i\" with points $i";
	if ($i != $numsources){
		$plotstring_cwnd = $plotstring_cwnd . ",";
	}
}
print $plotstring_cwnd;

$plotstring_fairalloc = " \"$fair_alloc\" title \"Fair_throughput\",";
for($i=1,$j=2; $i<=$numsources ; $i++,$j++){
	$plotstring_fairalloc = $plotstring_fairalloc . " \"$fair_alloc$i\" title \"flow$i\" ";
	if ($i != $numsources){
		$plotstring_fairalloc = $plotstring_fairalloc . ",";
	}
}
print $plotstring_fairalloc;

# Start up a gnuplot process and send the commands up until the END marker to
# it.  This puts out the plots.  To figure out what the commands do, the best
# plan is to use gnuplots help command.  E.g. "help set terminal postscript"
# from the gnuplot command line.
open(GNUPLOT, "|gnuplot") || die "Can't exec gnuplot: $!\n";
print GNUPLOT <<END;
set terminal postscript landscape color
set output "$stem.ps"
set key
set data style points
set title "Sequence number vs. time $graphtype"
set xlabel "Time (s)"
set ylabel "Sequence number (packets)"
#plot "$seqno_data" title "forwarded" with points 1, "$drop_data"  title "dropped" with points 2
#plot "$seqno_data1" title "flow1" with points 1, "$seqno_data2"  title "flow2" with points 2
plot $plotstring_seqno

set xrange[*:*]
set data style points
set title "Congestion Window vs. time $graphtype"
set xlabel "Time (s)"
set ylabel "Congestion Window (packets)"
#plot "$cwnd_data1" title "flow1" with points 1, "$cwnd_data2"  title "flow2" with points 2
set key
plot $plotstring_cwnd

set data style points
set title "Queue Length vs. time $graphtype"
set xlabel "Time (s)"
set ylabel "Queue length (packets)"
set nokey
plot "$queue_data"

set data style lines
set title "Bottleneck link utilization vs. time $graphtype"
set xlabel "Time (s)"
set ylabel "Link Utilization (%)"
plot "$util_data"

set data style points 
set title "Jain's Fairness Index Vs Time $graphtype"
set xlabel "Time (s)"
set ylabel "Jain's Fairness Index / Second"
plot "$fairness_data"

set data style lines 
set title "Throughput Fairness Vs Time $graphtype"
set xlabel "Time (s)"
set ylabel "Throughput (packets/second)"
set key
plot $plotstring_fairalloc



quit
END
close(GNUPLOT);
# To Clean up temp files, Uncomment the unlink command
#unlink($queue_data, $seqno_data, $drop_data, $cwnd_data);
