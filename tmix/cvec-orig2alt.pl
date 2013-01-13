#!/usr/bin/perl

#
# cvec2ns.pl - convert from Felix's connection vector format to a format more
#              suitable for reading by tmix-ns
#            - replace t, >, < lines with line with format:
#               * {I,A} - initator or acceptor
#               * time to wait after sending (usecs)
#               * time to wait after receiving (usecs)
#               * bytes to send (if 0, send a FIN)
#
# EXAMPLES: 
#  > 826             I   0    0    826
#  t 534             A   0    534  1213
#  < 1213            A   872  0    0
#  t 872
#
#  c< 190            A   0    0    190
#  t< 505            I   0    0    396
#  c> 396            A   505  0    46
#  t> 6250           I   6250 0    0
#  c< 46             
#

$dir == "";

while (<STDIN>) {
    # code from Perl Cookbook pg. 299
    chomp;               # remove newline
    s/^\s+//;            # remove leading whitespace
    s/\s+$//;            # remove trailing whitespace
    s/^SEQ/S/;           # replace SEQ with S
    s/^CONC/C/;          # replace CONC with C
    next unless length;  # anything left?

    # print out all header info
    if (/S|C|w|r|l/) {
	if (!$header) {
	    # this is the first line in header, finish up last entry

	    # print SEQ FIN, if necessary
	    print "$lastDir $time 0 0\n", if ($time > 0);
	    
	    # print CONC FIN, if necessary
	    print "I $initTime 0 0\n", if ($initTime > 0);
	    print "A $accTime 0 0\n", if ($accTime > 0);
	    
	    # reset vars
	    $time = 0;
	    $initTime = 0;
	    $accTime = 0;
	    $lastDir = -1;
	    $header = 1;
	}
	print $_, "\n";        	# print header line
	next;
    }
    
    if (/#.*/) {
	print $_, "\n";
	next;
    }

    # nothing after this point should be header
    die "ERROR> still in header: $_" unless /<|t|>/;

    # rest of entry is 'dir size' or 't usecs'
    ($sym, $val) = split;

    if (/^([><])/) {
	# SEQ initiator or acceptor sending
	$size = $val;
	$dir = "I", if ($1 eq ">");   # initiator
	$dir = "A", if ($1 eq "<");   # acceptor

	if ($lastDir eq $dir) {
	    # the last data was sent in the same dir, so send
	    # next data after $time
	    $sendTime = $time;
	    $recvTime = 0;
	}
	else {
	    # wait to receive before sending new data
	    $sendTime = 0;
	    $recvTime = $time;
	}
	$time = 0;         # reset time
	$lastDir = $dir;   # save this dir
	print "$dir $sendTime $recvTime $size\n";
    }
    elsif ($sym eq "t") {
	# SEQ time
	$time = $val;
	if ($time == 0) {
	    # if time is 0, then we can't distinguish between sending
	    # immediately and not waiting for a receive
	    $time = 1;
	}
    }
    elsif (/^((c>)|(c<))/) {
	# CONC direction and size
	$size = $val;

	if ($1 eq "c>") {
	    # initiator
	    $dir = "I";
	    print "$dir $initTime 0 $size\n";
	    $initTime = 0;
	}
	elsif ($1 eq "c<") {
	    # acceptor
	    $dir = "A";
	    print "$dir $accTime 0 $size\n";
	    $accTime = 0;
	}
    }
    elsif ($sym eq "t<") {
	# CONC acceptor time
	$accTime = $val;
	if ($accTime == 0) {
	    # if time is 0, then we can't distinguish between sending
	    # immediately and not waiting for a receive
	    $accTime = 1;
	}
    }
    elsif ($sym eq "t>") {
	# CONC initiator time
	$initTime = $val;
	if ($initTime == 0) {
	    # if time is 0, then we can't distinguish between sending
	    # immediately and not waiting for a receive
	    $initTime = 1;
	}
    }
    $header = 0;
}

# catch very last FIN

# print SEQ FIN, if necessary
print "$lastDir $time 0 0\n", if ($time > 0);
	    
# print CONC FIN, if necessary
print "I $initTime 0 0\n", if ($initTime > 0);
print "A $accTime 0 0\n", if ($accTime > 0);
	    
# print FIN if no time delay given
print "$dir 1 0 0\n", if ($time==0 && $initTime==0 && $accTime==0 && $dir ne "");
