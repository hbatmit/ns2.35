#!/home/johnh/BIN/perl5 -w

#
# DbGetopt.pm
# Copyright (C) 1995-1998 by John Heidemann <johnh@ficus.cs.ucla.edu>
# $Id: DbGetopt.pm,v 1.2 2005/09/16 04:41:55 tomh Exp $
#
# This program is distributed under terms of the GNU general
# public license, version 2.  See the file COPYING
# in $dblib for details.
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
#

package DbGetopt;

=head1 NAME

DbGetopt -- the currently preferred method of parsing args in jdb


=head1 SYNOPSIS

    use DbGetopt;
    $opts = new DbGetopts("ab:", \@ARGV);
    while ($opts->getopt()) {
	if ($opts->opt eq 'b') {
	    $b = $opts->optarg;
	}
    };
    @other_args = $opts->rest;

=head1 CREDITS

Taken from:

getoptk.pl -- getopt-like processing for Perl scripts, by
Brian Katzung  12 June 1993
<katzung@katsun.chi.il.us>,
and much hacked.

Perl5-ized by John Heidemann <johnh@isi.edu>.

=cut
#'


require 5.000;
require Exporter;
@EXPORT = qw();
@EXPORT_OK = qw();
($VERSION) = ('$Revision: 1.2 $' =~ / (\d+.d+) /);

use Carp qw(croak);


=head2 new("optionslist", \@ARGV)

Instantiate a new object.

=cut
#' font-lock hack
sub new {
    my($class) = shift @_;
    my($options, $optlistref) = @_;

    croak("DbGetopt::new: no options.\n") if (!defined($options));
    croak("DbGetopt::new: no option list or wrong type.\n") if (!defined($optlistref) || ref($optlistref) ne 'ARRAY');

    my $self = bless {
	opt => undef,
	opterr => 1,
	optarg => undef,
	_nextopt => '',
	_spec => $options,
	_optlistref => $optlistref,
    }, $class;
    return $self;
}

# from LWP::MemberMixin
sub _elem {
    my($self, $elem, $val) = @_;
    my $old = $self->{$elem};
    $self->{$elem} = $val if defined $val;
    return $old;
}
sub _elem_array {
    my($self) = shift @_;
    my($elem) = shift @_;
    return wantarray ? @{$self->{$elem}} : $self->{$elem}
        if ($#_ == -1);
    if (ref($_[0])) {
        $self->{$elem} = $_[0];
    } else {
	$self->{$elem} = ();
	push @{$self->{$elem}}, @_;
    };
    # always return array refrence
    return $self->{$elem};
}

=head2 opt, optarg, opterr, rest

Return the currently parsed option, that options's argument,
the error status, or any remaining options.

=cut
# '
sub opt { return shift->_elem('opt', @_); }
sub opterr { return shift->_elem('opterr', @_); }
sub optarg { return shift->_elem('optarg', @_); }
sub rest { return shift->_elem_array('_optlistref', @_); }

=head2 getopt

Get the next option, returning undef if out.

=cut
sub getopt {
    my($self) = shift;
    my($withArgs) = $self->{_spec};
    my($next);
    my($option, $i);
    my($argvref) = $self->{_optlistref};

    #
    # Fetch the next option string if necessary.
    #
    if (($next = $self->{'_nextopt'}) eq '') {
	#
	# Stop if there are no more arguments, if we see '--',
	# or if the next argument doesn't look like an option
	# string.
	#
	return undef
	    if (($#{$argvref} < 0) || (${$argvref}[0] eq '-') || (${$argvref}[0] !~ /^-/));
	if (${$argvref}[0] eq '--') {
	    shift(@{$argvref});
	    return undef;
	}
	
	#
	# Grab the next argument and remove it from @ARGV.
	#
	$next = shift @{$argvref};
	$next = substr($next, 1);
    };

    #
    # Peel off the next option.
    #
    $option = substr($next, 0, 1);
    $next = substr($next, 1);

    $i = index($withArgs, $option);
    if ($i == -1) {
	#
	# Unknown option.
	#
	croak("unknown option '$option'") if ($self->{'opterr'});
	# # put the argument back on ARGV
	# unshift (@ARGV, "-$option$next");
	$self->{'opt'} = '?';
	return 1;
    };
    if (substr($withArgs, $i+1, 1) eq ':') {
	#
	# The option takes an argument.  Take the argument
	# from the remainder of the option string, or use
	# the next argument if the option string is empty.
	#
	if ($next ne '') {
	    $self->{'optarg'} = $next;
	    $next = '';
	} else {
	    $self->{'optarg'} = shift(@{$argvref});
	};
    };

    #
    # Save the remainder of the option string and return
    # the current option.
    #
    $self->{'_nextopt'} = $next;
    $self->{'opt'} = $option;
    return 1;
}

=head2 ungetopt

Push the current option back on the options stream.
(May not exactly preserve original option parsing.)

=cut
sub ungetopt {
    my($self) = shift;
    my($opt) = $self->{'opt'};
    $opt .= $self->{_nextopt} if ($self->{_nextopt} ne '');
    unshift @{$self->{'_optlistref'}}, "-$opt";
}

# suppress warnings
my($bogus) = $VERSION;

1;
