#!/usr/bin/perl

use NS::TraceFileEvent qw(&string_to_hashref &quote_if_needed);
use NS::TraceFileWriter;


$output = new NS::TraceFileWriter();

my $lasttime = '*'; # keep track of most recent time value;

while (<>) {
	my ($time, $data);
	my ($type, $rest) = split(' ', $_, 2);
	if ($type =~ m/^#/) {
		# comment line.. might not be parsable.
		# we will only try to extract the time, and
		# quote everything else.

		if ($rest =~ m/^()-t\s+(\S+)\s*(.*)/ or
		    $rest =~ m/(.*?)-t\s+(\S+)\s*(.*)/) {
			$lasttime = $time = $2;
			$rest = $1.$3;
		} else {
			# couldn't find any time, so use the last time seen
			$time = $lasttime;
			chomp $rest;
		}
		$data = {'#' => quote_if_needed($rest)};
	} else {
	# ***XXX*** need to add support for "v" lines
		# not a comment, so go ahead and parse it
		$data = string_to_hashref($rest);
		$lasttime = $time = $data->{'t'};
		delete $data->{'t'};
	}
	$output->put_event(new NS::TraceFileEvent ($type,$time,%$data));
}
