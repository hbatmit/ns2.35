sub plotPreamble {
    my($OUT, $title, $xlabel, $xrange, $ylabel, $yrange) = @_;
#    if (defined($ENV{'DISPLAY'})) {
# 	writeln('set terminal x11');
#    } else {
# 	writeln('set terminal postscript');
# 	writeln('set output "', $file, '.ps"');
#    }
    print $OUT <<EOH
TitleText: $title
Device: Postscript
BoundBox: true
Ticks: true
Markers: true
XUnitText: $xlabel
YUnitText: $ylabel
EOH
    ;
    print $OUT "NoLines: true\n"	if (defined($opt_q));
}

sub plot {
    my($OUT, $append, $file, $ins, @vals) = @_;
    if ($ins =~ /plot\s+(\d+)\s*/) {
	$point = shift @vals;
	$max = $1 - 1;
	foreach $i (1..$max) {
	    plot($OUT, $append, "skip-$i", '', $point);
	}
	plot($OUT, $append, 'skip-' . ++$max, '', $point)	if ($#vals < $[);
    }
    print $OUT "\n\"$file\n", @vals if ($#vals >= $[);
}

%linkFails = %linkRecov = ();

sub plotFails {
    my($OUT, $minY, $maxY, $tag, @vals) = @_;
    if ($#vals >= $[) {
	foreach $i (@vals) {
	    push(@{$linkFails{$tag}}, "move $i $minY\n", "draw $i $maxY\n");
	}
    }
}

sub plotRecov {
    my($OUT, $minY, $maxY, $tag, @vals) = @_;
    if ($#vals >= $[) {
	foreach $i (@vals) {
	    push(@{$linkRecov{$tag}}, "move $i $minY\n", "draw $i $maxY\n");
	}
    }
}

sub plotPostamble {
    my($OUT) = shift @_;
    print $OUT "Nolines: true\n";
    foreach $i (keys %linkFails) {
	plot($OUT, 0, 'link ' . $i . ' fail', '', @{$linkFails{$i}});
    }
    foreach $i (keys %linkRecov) {
	plot($OUT, 0, 'link ' . $i . ' recovery', '', @{$linkRecov{$i}});
    }
    close(OUT);
}

1;
