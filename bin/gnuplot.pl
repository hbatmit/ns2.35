$file = '';
sub plotPreamble {
    my($OUT, $title, $xlabel, $xrange, $ylabel, $yrange) = @_;
#    if (! defined($ENV{'DISPLAY'})) {
#	print $OUT 'set terminal dumb', "\n";
#	print $OUT 'set output "/dev/null"', "\n";
#    }
    $file = $title;
    print $OUT "set title  '$title'   \n";
    print $OUT "set xlabel '$xlabel'  \n";
    print $OUT "set ylabel '$ylabel'  \n";
    print $OUT "set xrange [$xrange]  \n"	if ($xrange);
    print $OUT "set yrange [$yrange]  \n"	if ($yrange);
}

$plotcmd = 'plot';

sub plot {
    my($OUT, $append, $file, $ins, @vals) = @_;
    if ($#vals >= $[) {
	open(DOUT, ($append ? ">>$file" : ">$file"));
	print DOUT @vals;
	close(DOUT);
	print $OUT "$plotcmd '$file' $ins\n";
	$plotcmd = 'replot';
    }
}

$labelid = '000';
sub setLabel {
    my($OUT, $tag, $label, $x, $y, $pos) = @_;
    print $OUT "set label ${labelid}$tag '$label' at $x, $y $pos\n";
    ++$labelid;
}

sub setArrow {
    my($OUT, $tag, $x1, $y1, $x2, $y2) = @_;
    print $OUT "set arrow ${labelid}$tag from $x1, $y1 to $x2, $y2\n";
    ++$labelid;
}

$ffirst = 0;
sub plotFails {
    my($OUT, $minY, $maxY, $tag, @vals) = @_;
    if ($#vals >= $[) {
	my($range, $start, $tick1, $tick2, @nvals);
	$range = $maxY - $minY;
	$start = $maxY - $range * 0.1;
	$tick1 = $maxY - $range * 0.2;
	$tick2 = $maxY - $range * 0.22;
	foreach $i (@vals) {
	    push(@nvals, "$i $minY\n", "$i $maxY\n");
	    setArrow($OUT, $tag, $i, $start, $i, $tick1);
	    setArrow($OUT, $tag, $i, $start, $i, $tick2);
	    setLabel($OUT, $tag, "<$tag>",  $i - 0.11, $tick2, 'right');
	}
	plot($OUT, $ffirst, 'link-fails', 'w impulse', @nvals);
	$ffirst = 1;
    }
}

$rfirst = 0;
sub plotRecov {
    my($OUT, $minY, $maxY, $tag, @vals) = @_;
    if ($#vals >= $[) {
	my($range, $start, $tick1, $tick2, @nvals);
	$range = $maxY - $minY;
	$start = $maxY - $range * 0.3;
	$tick1 = $maxY - $range * 0.1;
	$tick2 = $maxY - $range * 0.12;
	foreach $i (@vals) {
	    push(@nvals, "$i $minY\n", "$i $maxY\n");
	    setArrow($OUT, $tag, $i, $start, $i, $tick1);
	    setArrow($OUT, $tag, $i, $start, $i, $tick2);
	    setLabel($OUT, $tag, "<$tag>", $i + 0.11, $tick1, 'left');
	}
	plot($OUT, $rfirst, 'link-recovery', 'w impulse', @nvals);
	$rfirst = 1;
    }
}

sub plotPostamble {
    my($OUT) = shift @_;
#    print $OUT "set terminal postscript\n";
#    print $OUT "set output '${file}.ps'\n";
#    print $OUT "replot\n";
#    print $OUT "set output\n";
    print $OUT "exit\n";
    close(OUT);
}

1;
