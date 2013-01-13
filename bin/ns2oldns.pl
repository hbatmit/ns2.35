#!/usr/bin/perl

use NS::TraceFileEvent qw(&hashref_to_string);
use NS::TraceFileReader;

my $input = new NS::TraceFileReader();
my $event;

while ($event = $input->get_event()) {
	my $type = $event->get_type();

	if ($type =~ m/^[-+rd]$/) {
		my $x = $event->get('x');
		$x =~ s/^{(.*)}$/$1/;
		my @x = split ' ', $x;
		print $type, " ", $event->get_timestamp(), " ",
		      $event->get('s'), " ",
		      $event->get('d'), " ",
		      $event->get('p'), " ",
		      $event->get('e'), " ",
		      $x[3], " ",
		      $event->get('c'), " ",
		      $x[0], " ",
		      $x[1], " ",
		      $x[2], " ",
		      $event->get('i'), "\n";
	} elsif ($type =~ m/^#/) {
	# it's a comment, so new style output is ok :)
		print $event->get_string_representation() . "\n";
	}
}
