#!/usr/bin/perl

use NS::TraceFileEvent qw(&hashref_to_string);
use NS::TraceFileReader;

my $input = new NS::TraceFileReader();
my $event;

while ($event = $input->get_event()) {
# ***XXX*** need to modify to handle "v" event type, which is weird
    if ($event->get_type() =~ m/^#/) {
    	# it's a comment, new style output is ok :)
	print $event->get_string_representation() . "\n";
    } else {
        print $event->get_type(), " -t ", $event->get_timestamp(), " ",
              hashref_to_string($event->get_data()), "\n";
    }
}
