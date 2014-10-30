
# Perl package for representing events in NS trace files


# ***XXX*** Need to work out comments.  Probably make it so that comments
#           result in single data item called comment

package NS::TraceFileEvent;

use 5.005;
use strict;
use warnings;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

BEGIN {
	use Text::Balanced qw(extract_bracketed);
	use Exporter ();


	$VERSION = 1.00;

	@ISA = qw(Exporter);
	@EXPORT = qw();
	%EXPORT_TAGS = ();
	@EXPORT_OK = qw(&string_to_hashref &hashref_to_string &quote_if_needed);
}

# the constructor:
sub new {
	my $self  = shift;
	my $class = ref($self) || $self;
	my $obj = { type => undef,
		    timestamp => undef,
		    data => {}
		  };

	bless $obj, $class;  # make the hash into an object


	# if there are still more arguments to process, then use them
	# to initialize the object.  Otherwise, object will be uninitialized.
	if (scalar(@_) == 1) {
		# init with string
		$obj = $obj->set_string_representation(shift);
	} elsif (@_) {
		# init with data
		$obj->set_type(shift);
		$obj->set_timestamp(shift);
		if (ref $_[0] eq 'HASH') {
			$obj->set_data(shift);
		} else {
			$obj->set_data({@_});
		}
	}

	return $obj;
}


# get the type of event
sub get_type {
	my $self = shift;
	return $self->{'type'};
}

# set the type of the event
sub set_type {
	my ($self, $type) = @_;
	$self->{'type'} = $type;

	# invalidate string representation
	delete $self->{'string_representation'};
}

# get the timestamp for the event
sub get_timestamp {
	my $self = shift;
	return $self->{'timestamp'};
}

# set the timestamp of the event
sub set_timestamp {
	my ($self, $time) = @_;
	$self->{'timestamp'} = $time;

	# invalidate string representation
	delete $self->{'string_representation'};
}

# get a value from the event
sub get {
	my ($self, $key) = @_;
	return $self->{'data'}{$key}
}

# set a value in the event
sub set {
	my ($self, $key, $value) = @_;
	$self->{'data'}{$key} = $value;

	# invalidate string representation
	delete $self->{'string_representation'};
}

# remove a value from the event
sub remove {
	my ($self, $key) = @_;
	delete $self->{'data'}{$key};
	
	# invalidate string representation
	delete $self->{'string_representation'};
}

# get all data
sub get_data {
	my $self = shift;
	return $self->{'data'};
}

# set all data
sub set_data {
	my $self = shift;
	my $new_data = shift;
	if (defined $new_data and ref $new_data ne 'HASH') {
		die "NS::TraceFileEvent::set_data wants a hash reference";
	}
	$self->{'data'} = $new_data;

	# invalidate string representation
	delete $self->{'string_representation'};
}

# return the string representation of the event, suitable for writing
# to an ns trace file
sub get_string_representation {
	my $self = shift;

	unless (defined $self->{'string_representation'}) {
		# construct a string representation if none is cached

		unless (defined $self->{'type'} and
			defined $self->{'timestamp'}) {
			return undef; # abort if object is not initialized
		}

		if ($self->{'type'} =~ m/^#/ and
		    defined $self->{'data'}{'#'} and
		    (keys %{$self->{'data'}} == 1)) {
	 		# freeform comment line
			$self->{'string_representation'} =
		    		$self->{'type'} . ' ' . $self->{'data'}{'#'};
		} else {
			# normal event
			$self->{'string_representation'} =
				$self->{'type'} . ' ' .
				$self->{'timestamp'} . ' ' .
				hashref_to_string($self->{'data'});
		}
	}

	return $self->{string_representation};
}

# set all data via string representation
# returns undef if operation fails (string was malformed) and returns
# the object itself if the operation succeeds.
sub set_string_representation {
	my ($self, $string) = @_;

	chomp $string; # remove any trailing newline
	$self->{'string_representation'} = $string; # save a copy

	my ($type, $time, $data) = split ' ', $string, 3;
	$self->{'type'} = $type;

	if ($type =~ m/^#/) {
		# we don't try to parse the comment
		$self->{'timestamp'} = '*';
		$self->{'data'} = {'#' => "$time $data"};
	} else {
		unless (defined $type and defined $time) {
			# event is malformed if type or time is malformed
			$self->{'type'} = undef;
			$self->{'timestamp'} = undef;
			$self->{'data'} = {};
			delete $self->{'string_representation'};
			return undef;
		}

		$self->{'timestamp'} = $time;
		$self->{'data'} = string_to_hashref($data);
	}

	return $self;
}




# exportable functions


sub quote_if_needed {
	my $value = shift;

	# if delimited in brackets or quotes already, then don't change
	if ($value =~ m/^(".*"|{.*})$/) {
		return $value;
	}

	#otherwise, see if it needs quoting
	if ($value =~ m/ / or
	    $value eq '') {
		$value = "{$value}";
	}

	return $value;
}


sub string_to_hashref {
	my $string = shift;
	my ($tag, $value, %hash);

	while (defined $string and
	       ($tag, $string) = split(' ', $string, 2)) {
		$tag =~ s/-(.*)/$1/;
		if ($string =~ m/^({|\[|")/) {
			# if the value appears to be quoted, then
			# find the end of the quote
			($hash{$tag}, $string) = extract_bracketed($string,
								   '{["');
		} else {
			# value doesn't appear to be quoted, so whitespace
			# will mark the end of the value
			($hash{$tag}, $string) = split(' ', $string, 2);
		}
	}

	return \%hash;
}

sub hashref_to_string {
    my $ref = shift;
    my %data = %$ref;
    return join (' ', map { "-$_ ". quote_if_needed($data{$_}) }
	                  keys %data);
}




# a Perl module must return a true value
1;

