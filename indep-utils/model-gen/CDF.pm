
###############################################
#  A simple Perl module for using CDF files
#
#  Tim Buchheim,  25 September 2002
#
#  based on C++ code found in the ns project
#
# File format:
#
#  first column:  value
#  second column: cumulative number of occurances (ignored)
#  third column:  cumulative probability
#
###############################################

package CDF;

use strict;


# the constructor
#
#  $foo = new CDF($filename);
#

sub new {
    my $class = shift;
    my $i = 0;
    my $file = shift;
    my @table;

    open INPUT_FILE, $file or die "Unable to open file: $file";
    while (<INPUT_FILE>) {
        my ($value, $num, $prob) = split;
	$table[$i] = [$prob, $value];
	++$i;
    }
    close INPUT_FILE;
    return bless \@table, $class;
}




# public methods

#
#  $foo->value();
#
# looks up the value for a random number.  Does not do any interpolation.
sub value {
    my $self = shift;
    my @table = @$self;

    if (scalar(@table) <= 0) { return 0; }

    my $u = rand;
    my $mid = $self->lookup($u);

    return $table[$mid][1];
}


#
#  $foo->interpolated_value();
#
# looks up the value for a random number.  Interpolates between table
# entries.
sub interpolated_value {
    my $self = shift;
    my @table = @$self;

    if (scalar(@table) <= 0) { return 0; }

    my $u = rand;
    my $mid = $self->lookup($u);
    if ($mid and $u < $table[$mid][0]) {
        return interpolate($u, $table[$mid-1][0], $table[$mid-1][1],
	                   $table[$mid][0], $table[$mid][1]);
    }
    return $table[$mid][1];
}


# private method
sub lookup {
    my $self = shift;
    my @table = @$self;
    my $u = shift;
    if ($u <= $table[0][0]) {
    	return 0;
    }
    my ($lo, $hi, $mid);
    for ($lo = 1, $hi = scalar(@table) - 1; $lo < $hi; ) {
        $mid = ($lo + $hi) / 2;
	if ($u > $table[$mid][0]) {
	    $lo = $mid + 1;
	} else {
	    $hi = $mid;
	}
    }
    return $lo;
}

# private function
sub interpolate {
    my ($x, $x1, $y1, $x2, $y2) = @_;
    my $value = $y1 + ($x - $x1) * ($y2 - $y1) / ($x2 - $x1);
    return $value;
}





# a Perl package must return true
1;

