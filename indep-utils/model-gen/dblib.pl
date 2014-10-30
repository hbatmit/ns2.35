#!/home/johnh/BIN/perl5

#
# dblib.pl
# Copyright (C) 1991-1998 by John Heidemann <johnh@isi.edu>
# $Id: dblib.pl,v 1.2 2005/09/16 04:41:55 tomh Exp $
#
# This program is distributed under terms of the GNU general
# public license, version 2.  See the file COPYING
# in $dblibdir for details.
# 
# The copyright of this module includes the following
# linking-with-specific-other-licenses addition:
# 
# In addition, as a special exception, the copyright holders of
# this module give you permission to combine (via static or
# dynamic linking) this module with free software programs or
# libraries that are released under the GNU LGPL and with code
# included in the standard release of ns-2 under the Apache 2.0
# license or under otherwise-compatible licenses with advertising
# requirements (or modified versions of such code, with unchanged
# license).  You may copy and distribute such a system following the
# terms of the GNU GPL for this module and the licenses of the
# other code concerned, provided that you include the source code of
# that other code when and as the GNU GPL requires distribution of
# source code.
# 
# Note that people who make modified versions of this module
# are not obligated to grant this special exception for their
# modified versions; it is their choice whether to do so.  The GNU
# General Public License gives permission to release a modified
# version without this exception; this exception also makes it
# possible to release a modified version which carries forward this
# exception.
#

$col_headertag = "#h";
$list_headertag = "#L";
$headertag_regexp = "#[hL]";

$fs_code = 'D';
$header_fsre = "[ \t\n]+";
$fsre = "[ \t\n]+";
$outfs = "\t";
$header_outfs = " ";
$codify_code = "";
$default_format = "%.5g";

sub col_mapping {
    local ($key, $n) = @_;

    die("dblib col_mapping: column name ``$key'' cannot begin with underscore.\n")
    	if ($key =~ /^\_/);
    die("dblib col_mapping: duplicate column name ``$key''\n")
    	if (defined($colnametonum{$key}));
    die ("dblib col_mapping: bad n.\n") if (!defined($n));
    $colnames[$n] = $key;
    $colnametonum{$key} = $n;
    $colnametonum{"_$key"} = $n;
    $colnametonum{"$n"} = $n;   # numeric synonyms
}

sub col_unmapping {
    local ($key) = @_;
    local ($n);
    $n = $colnametonum{$key};
    $colnames[$n] = undef if (defined($n));
    delete $colnametonum{$key};
    delete $colnametonum{"_$key"};
}

# Create a new column.
# Insert it before column $desired_n.
sub col_create {
    local ($key, $desired_n) = @_;
    local ($n, $i);
    die ("dblib col_create: called with duplicate column name ``$key''.\n")
    	if (defined($colnametonum{$key}));
    if (defined($desired_n)) {
    	# Shift columns over as necessary.
    	$n = $colnametonum{$desired_n};
	for ($i = $#colnames; $i >= $n; $i--) {
	    $tmp_key = $colnames[$i];
	    &col_unmapping($tmp_key);
	    &col_mapping($tmp_key, $i+1);
	};
    } else {
    	$n = $#colnames+1;
    };
    $colnames[$n] = $key;
    &col_mapping ($colnames[$n], $n);
    return $n;
}

sub fs_code_to_fsre_outfs {
    my($value) = @_;
    my($fsre, $outfs);
    if (!defined($value) || $value eq 'D') {  # default
	$fsre = "[ \t\n]+";
	$outfs = "\t";
    } elsif ($value eq 'S') {   # double space
	$fsre = '\s\s+';
	$outfs = "  ";
    } elsif ($value eq 't') {   # single tab
	$fsre = "\t";
	$outfs = "\t";
    } else {   # anything else
	$value = eval "qq{$value}";  # handle backslash expansion
	$fsre = "[$value]+";
	$outfs = $value;
    }
    return ($fsre, $outfs);
}

sub process_header {
    my($line, $headertag) = @_;
    $regexp = (defined($headertag) ? $headertag : $headertag_regexp);

    die ("dblib process_header: undefined header.\n")
	if (!defined($line));
    die ("dblib process_header: invalid header format: ``$line''.\n")
        if ($line !~ /^$regexp/);
    @colnames = split(/$header_fsre/, $line);
    shift @colnames;   # toss headertag
    @coloptions = ();
    #
    # handle options
    #
    while ($#colnames >= 0 && $colnames[0] =~ /^-(.)(.*)/) {
	push(@coloptions, shift @colnames);
	my($key, $value) = ($1, $2);
	if ($key eq 'F') {
	    ($fsre, $outfs) = fs_code_to_fsre_outfs($value);
	    $fs_code = $value;
	};
    };
    %colnametonum = ();
    foreach $i (0..$#colnames) {
    	&col_mapping ($colnames[$i], $i);
    };
}

sub readprocess_header {
    my($headertag) = @_;
    my($line);
    $line = <STDIN>;
    &process_header($line, $headertag);
}

sub write_header {
    my(@cols) = @_;
    @cols = @colnames if ($#cols == -1);
    print "$col_headertag$header_outfs" .
	($#coloptions != -1 ? join($header_outfs, @coloptions, '') : "") .
	join($header_outfs, @cols) .
	"\n";
}

# listized
sub write_list_header {
    local (@cols) = @_;
    @cols = @colnames if ($#cols == -1);
    print "$list_headertag $outfs" .
	join($outfs, @cols) .
	"\n";
}

sub escape_blanks {
    my($line) = @_;
    $line =~ s/[ \t]/_/g;
    return $line;
}

sub unescape_blanks {
    my($line) = @_;
    $line =~ s/_/ /g;
    return $line;
}

#
# codify:  convert db-code into perl code
#
# The conversion is a rename of all _foo's into
# database fields.
# For more perverse needs, _foo(N) means the Nth field after _foo.
# To convert we eval $codify_code.
#
# NEEDSWORK:  Should make some attempt to catch misspellings of column
# names.
#
sub codify {
    if ($codify_code eq "") {
        foreach (@colnames) {
	    $codify_code .= '$f =~ s/\b\_' . quotemeta($_) . '(\(.*\))/\$f\[' . $colnametonum{$_} . '+$1\]/g;' . "\n";
	    $codify_code .= '$f =~ s/\b\_' . quotemeta($_) . '\b/\$f\[' . $colnametonum{$_} . '\]/g;' . "\n";
        };
    };
    local($f) = join(";", @_);
    eval $codify_code;
    return $f;
}

#
# code_prettify:  Convert db-code into "pretty code".
#
sub code_prettify {
    local($prettycode) = join(";", @_);
    $prettycode =~ s/\n/ /g;   # newlines will break commenting
    return $prettycode;
}

sub is_comment {
    return ($_ =~ /^\#/) || ($_ =~ /^\s*$/);
}

sub pass_comments {
    if (&is_comment) {
	print $_;
	return 1;
    };
    return 0;
}

sub delayed_pass_comments {
    if (&is_comment) {
	$delayed_comments = (!defined($delayed_comments) ? '' : $delayed_comments) . $_;
	return 1;
    };
    return 0;
}
sub delayed_flush_comments {
    print $delayed_comments if (defined($delayed_comments));
    $delayed_comments = undef;
}

sub split_cols {
    chomp $_;
    @f = split(/$fsre/, $_);
}

sub write_cols {
    print join($outfs, @f), "\n";
};

sub write_these_cols {
    print join($outfs, @_), "\n";
};

#
# output compare/entry code based on ARGV
# first entry is a sub:
#	sub row_col_fn {
#	    my($row, $colname, $n) = @_;
#	    # row is either a or b which we're comparing, or i for entries
#	    # colname is the user-given column name
#	    # n is 0..N of the cols to be sorted
#	}
# See the code in dbjoin and dbsort for implementations.
#
sub generate_compare_code {
    my($compare_function_name) = shift @_;
    my($row_col_fn) = shift @_;
    my(@args) = @_;
    my ($compare_code, $enter_code, $reverse, $numeric, $i);
    $compare_code = "sub $compare_function_name {\n";
    $enter_code = "";
    $reverse = 0;
    $numeric = 0;
    $i = 0;
    foreach (@args) {
        if (/^-/) {
	    s/^-//;
	    my($options) = $_;
	    while ($options ne '') {
		$options =~ s/(.)//;
		($ch) = $1;
	        if ($ch eq 'r') { $reverse = 1; }
	        elsif ($ch eq 'R') { $reverse = 0; }
	        elsif ($ch eq 'n') { $numeric = 1; }
	        elsif ($ch eq 'N') { $numeric = 0; }
	        else { die "dblib generate_compare_code: unknown option $ch.\n"; };
	    };
	    next;
        };
	die ("dblib generate_compare_code: unknown column $_.\n") if (!defined($colnametonum{$_}));
	if ($reverse) {
            $first = 'b';   $second = 'a';
        } else {
            $first = 'a';   $second = 'b';
        };
        $compare_code .= '$r = (' . &$row_col_fn($first, $_, $i) . ' ' . 
    	    	    ($numeric ? "<=>" : "cmp") .
		    ' ' . &$row_col_fn($second, $_, $i) . '); ' . 
		    'return $r if ($r);' . 
		    " # $_" .
			($reverse && $numeric ? " (reversed, numeric)" :
			$reverse ? " (reversed)" :
			$numeric ? " (numeric)" :
			"") .
		    "\n";
        $enter_code .= &$row_col_fn('i', $_, $i) . 
       		    ' = $f[' . $colnametonum{$_} . '];' . "\n";
	$i++;
    }
    $compare_code .= "return 0;\n}";
    # Create the comparison function.
    eval $compare_code;
    $@ && die("dblib generate_compare_code: error ``$@ in'' eval of compare_code.\n$compare_code");
    return ($compare_code, $enter_code, $i-1);
}


sub abs {
    return $_[0] > 0 ? $_[0] : -$_[0];
}

sub progname {
    my($prog) = ($0);
    $prog =~ s@^.*/@@g;
    return $prog;
}

sub force_numeric {
    my($value, $ignore_non_numeric) = @_;
    if ($value =~ /^[-+]?[0-9]+(.[0-9]+)?(e[-+0-9]+)?$/) {
        return $value + 0.0;   # force numeric
    } else {
	if ($ignore_non_numeric) {
	    return undef;
	    next;
	} else {
	    return 0.0;
	};
    };
}

my($tmpfile_counter) = 0;
my(@tmpfiles) = ();
# call as tmpfile(FH)
sub db_tmpfile {
    my($fh) = @_;
    my($i) = $tmpfile_counter++;
    my($fn) = &db_tmpdir . "/$$.$i";
    push(@tmpfiles, $fn);
    open($fh, "+>$fn") || die "$0: tmpfile open failed.\n";
    chmod 0600, $fn || die "$0: tmpfile chmod failed.\n";
    return $fn;
}

sub db_tmpdir {
    $ENV{'TMPDIR'} = '/tmp' if (!defined($ENV{'TMPDIR'}));
    return $ENV{'TMPDIR'};
}

my($dblib_date_inited) = undef;
sub dblib_date_init {
    eval "use HTTP::Date; use POSIX";
}

sub date_to_epoch {
    my($date) = @_;
    &dblib_date_init if (!$dblib_date_inited);
    return str2time($date);
}

sub epoch_to_date {
    my($epoch) = @_;
    &dblib_date_init if (!$dblib_date_inited);
    my($d) = strftime("%d-%b-%y", gmtime($epoch));
    $d =~ s/^0//;
    return $d;
}

sub epoch_to_fractional_year {
    my($epoch) = @_;
    &dblib_date_init if (!$dblib_date_inited);
    my($year) = strftime("%Y", gmtime($epoch));
    my($year_beg_epoch) = date_to_epoch("${year}0101");
    my($year_end_epoch) = date_to_epoch(($year+1) . "0101");
    my($year_secs) = $year_end_epoch - $year_beg_epoch;
    my($fraction) = ($epoch - $year_beg_epoch) / (1.0 * $year_secs);
    $fraction =~ s/^0//;
    return "$year$fraction";
}

sub END {
    foreach (@tmpfiles) {
	unlink($_) if (-f $_);
    };
}


1;
