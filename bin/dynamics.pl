
# requires either gnuplot.pl or xgraph.pl
#

%su = %lu = %sd = %ld = ();	# su/sd are HoL arrays with semantics:
				# key %su/%sd is the link that went up/down
				# values %su/%sd is implicitly hashed
				# @{${su{link}}/@{$sd{link}} is the list
				# 	of times link went up/down.
				# %lu/%ld are used to ensure that %su/%sd
				# 	is unique.

sub parseDynamics {
    my(@F) = @_;
    my($a, $b) = ($F[3], $F[4]);
    ($a, $b) = ($b, $a)		if ($a > $b);

    if ($F[2] eq 'link-down') {
	if (! defined($lf{"$a:$b:$F[1]"})) {
	    push(@{$sd{"$a$b"}}, $F[1]);
	    $lf{"$a:$b:$F[1]"} = 1;
	}
	return;
    }
    if ($F[2] eq 'link-up') {
	if (! defined($lu{"$a:$b:$F[1]"})) {
	    push(@{$su{"$a$b"}}, $F[1]);
	    $lu{"$a:$b:$F[1]"} = 1;
	}
	return;
    }
}	

sub plotDynamics {
    my($OUT, $minY, $maxY) = @_;
    foreach $i (keys %sd) {
        plotFails($OUT, $minY, $maxY, $i, @{$sd{$i}});
    }
    foreach $i (keys %su) {
        plotRecov($OUT, $minY, $maxY, $i, @{$su{$i}});
    }
}

1;
