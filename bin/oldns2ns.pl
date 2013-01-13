#!/usr/bin/perl

use NS::TraceFileEvent qw(&quote_if_needed);
use NS::TraceFileWriter;

my $output = new NS::TraceFileWriter();

my $lasttime = '*';  # keep track of most recent timestamp

while (<>) {
	my @val = split;

	if ($val[0] =~ m/^#/) {
		$output->put_event(new NS::TraceFileEvent(
				shift @val,
				$lasttime, # use most recent timestamp
				{ '#' => quote_if_needed(join ' ', @val) }));
	} elsif ($val[0] =~ m/^[-+hr]$/) {
		# packet info (queue/dequeue/drop)
		$lasttime = $val[1]; # save timestamp
		$output->put_event(new NS::TraceFileEvent(
					$val[0],
					$val[1],
					{
					  s => $val[2],
					  d => $val[3],
					  p => $val[4],
					  e => $val[5],
					  c => $val[7],
					  i => $val[11],
					  x =>
					  "{$val[8] $val[9] $val[10] $val[6] null}"
					}));
	} else {
		# other type of event.  ignore for now
		print STDERR "Event type \"$val[0]\" skipped.\n";
	}
}
