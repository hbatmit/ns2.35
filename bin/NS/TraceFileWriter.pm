
# Perl package for writing NS trace files

package NS::TraceFileWriter;

use 5.005;
use strict;
use warnings;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

BEGIN {
	use IO::Handle;
	use IO::File;
	use NS::TraceFileEvent;
	use Exporter ();


	$VERSION = 1.00;

	@ISA = qw(Exporter);
	@EXPORT = qw();
	%EXPORT_TAGS = ();
	@EXPORT_OK = qw();
}

# the constructors:

sub new {
	my $self  = shift;
	my $class = ref($self) || $self;
	my $hash = {};


	if (@_) {
		$hash->{filehandle} = IO::File->new(shift,"w");
	} else {
		# default to STDOUT
		$hash->{filehandle} = IO::Handle->new_from_fd(fileno(STDOUT),
							      "w");
	}
	return bless $hash, $class;
}

sub new_from_fd {
	my $self = shift;
	my $class = ref($self) || $self;
	my $hash = {};

	if (@_) {
		$hash->{filehandle} = IO::Handle->new_from_fd(fileno(shift),
							      "w");
	} else {
		print STDERR "Must supply filehandle to NS::TraceFileWriter->new_from_fd(\$filehandle).\n";
	}
}


# write an event to the file
sub put_event {

	my $self = shift;
	my $event = shift;

	return $self->{filehandle}->print($event->get_string_representation(),
					  "\n");
}

# a Perl module must return a true value
1;

