
# Perl package for reading NS trace files

package NS::TraceFileReader;

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
		$hash->{filehandle} = IO::File->new(shift,"r");
	} else {
		# default to STDIN
		$hash->{filehandle} = IO::Handle->new_from_fd(fileno(STDIN),
							      "r");
	}
	return bless $hash, $class;
}


sub new_from_fd {
	my $self = shift;
	my $class = ref($self) || $self;
	my $hash = {};

	if (@_) {
		$hash->{filehandle} = IO::Handle->new_from_fd(fileno(shift),
							      "r");
	} else {
		print STDERR "Must supply filehandle to NS::TraceFileReader->new_from_fd(\$filehandle).\n";
	}
}


# read and parse a line from the file
sub get_event {

	my $self = shift;

	my $line = $self->{filehandle}->getline;
	if (defined $line) {
		return new NS::TraceFileEvent $line;
	} else {
		return undef;
	}
}





# a Perl module must return a true value
1;

