# Created May 98 by Ahmed Helmy; updated June 98
# routing generator class
Class RouteGen

proc route-usage { } {
puts {usage: routing [-<key 1> <value 1> ... -<key n> <value n>]

example options: 
-outfile f -unicast DV -multicast detailedDM -expand_address off

keys and corresponding values:
-outfile [the output file that will contain the ns script 
	 describing the routing. This must be given.]

-unicast
  possible values: 
	Session (default) [centralized Dijkstra's SPF algorithm]
	DV [detailed/distributed Bellman-Ford algorithm]

-multicast [by default multicast is turned off, unless this
		option is given]
  possible values: 
	CtrMcast [centralized sparse multicast]
	detailedDM [detailed/distributed dense-mode multicast,
			based on PIM-DM]
	PIM [an early version of detailed/distributed sparse mode
		multicast, based on PIM-SM.]

-expand_address 
  possible values: on (default) , off

  }
}

proc routing { args } {
	set len [llength $args]

	if { $len } {
	    set key [lindex $args 0]
            if {$key == "-?" || $key == "--help" || $key == "-help" \
			|| $key == "-h" } {
			route-usage
			return
                }
	}

        if { [expr $len % 2] } {
                # if number is odd => error !
                puts "fewer number of arguments than needed in \"$args\""
                route-usage
		return
        }

	# check if the routeGen type exists
	if { [catch {set rg [RouteGen info instances]}] } {
		puts "no class RouteGen"
		route-usage
		return
	}
	if { $rg == "" } {
		set rg [new RouteGen]
	}
	if { ![llength $args] } {
		$rg create
	} else {
		$rg create $args
	}
}

RouteGen instproc init { } {
	$self next
}

RouteGen instproc default_options { } {

	$self instvar opt_info

	set opt_info {
		# init file to -1, must be supplied by input
		outfile -1

		# unicast default is centralized (i.e. rtproto
		# Session)
		unicast Session

		# multicast default is detailedDM ! is this a
		# good choice ?! or better centralized (i.e.
		# mproto CtrMcast
		# multicast centralized
		# default is multicast is turned off !!
		multicast -1

		# where does address assignment (e.g. expand, and
		# hierarchical) fit ?! XXX
		# by default turn them on
		expand_address on
		# hierarchical addressing is decided at topology
		# generation time... not here !
		# hierarchical-addressing on

	}
	$self parse_opts
}
	
# are the parsing functions general enuf... ! chk
RouteGen instproc parse_opts { } {
	$self instvar opts opt_info

	while { $opt_info != ""} {
		# parse line by line
                if {![regexp "^\[^\n\]*\n" $opt_info line]} {
                        break  
                }
		# remove the parsed line
                regsub "^\[^\n\]*\n" $opt_info {} opt_info
		# remove leading spaces and tabs using trim
                set line [string trim $line]
		# skip comment lines beginning with #
                if {[regexp "^\[ \t\]*#" $line]} {
                        continue
                }
		# skip empty lines
                if {$line == ""} {
                        continue
                } elseif [regexp {^([^ ]+)[ ]+([^ ]+)$} $line dummy key value] {
                        set opts($key) $value
                } 
	}
}

RouteGen instproc parse_input { args } {
	# remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]

	$self instvar opts

	for { set i 0 } { $i < $len } { incr i } {
		set key [lindex $args $i]
		regsub {^-} $key {} key
                if {![info exists opts($key)]} {
			puts stderr "unrecognized option $key"
			route-usage
			return -1
		}
		incr i
		# puts "changing $key from $opts($key) to [lindex $args $i]"
		set opts($key) [lindex $args $i]
	}
	# puts "end of parsing... "
	return 0
}

RouteGen instproc create { args } {
        # remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]
        # puts "calling create with args $args, len $len"

	$self default_options

	if { $len } {
		if { [$self parse_input $args] == -1 } {
			return 
		}
	}
	# check that the filename is provided
	$self instvar opts
	if { $opts(outfile) == -1 } {
	 puts {you must provide outfile name. use "routing -h" for help}
		return
	}
	$self create-routing
}

RouteGen instproc create-routing { } {
	# XXX leave out the checks to ns later.. !
	# this would allow extensibility to include other kinds of 
	# routing without having to modify the generator
	# if { [$self input-check] == -1 } {
	#	puts "There's an input error !! aborting"
	#	route-usage
	# 	return
	# }
	
	$self instvar opts

	set file $opts(outfile)
	set unicast $opts(unicast)
	set mcast $opts(multicast)
	set add $opts(expand_address)

	$self generate-code $file $unicast $mcast $add
}

# later may add other options.. for unicast multipath.. etc
RouteGen instproc generate-code { file unicast mcast add } {
	set f [open $file w]

	# puts "generate-code with $file $unicast $mcast $add"

	set aa "\n proc setup-mcastNaddr \{ sim \} \{ \n"
	set ab "\t upvar \$sim ns\n"

	set ac ""; set ad ""; set bd ""; set ae ""

	# add other params here later.. multipath..etc
	if { $mcast != -1 } {
	 	set ac "\t Simulator set EnableMcast_ 1\n"
		set ad "\t Simulator set NumberInterfaces_ 1\n"
		# check if the following has to be done 
		# only after the nodes are created !! XXX
	 # get a handle to mcast that may be used later for
	 # ex. to switch treetype... etc
	 set bd "\t set mrthandle \[\$ns mrtproto $mcast \{\}\]\n"
	} 
	if { $add == "on" } {
		set ae "\t Node expandaddr\n"
	} 

	set ba "\n proc create-routing \{ sim \} \{\n"
	set bb "\t upvar \$sim ns\n"

	set bc "\t \$ns rtproto $unicast \n"
	set c "\} \n"; # close the proc paren
	# XXX this is 
	# ns artifact that mcast enable should come before
	# topology generation and assigning uni/mcast
	# should come after !!!

	# generate the first procedure ... 
	set str1 "$aa $ab $ac $ad $ae $c"

	# generate the 2nd procedure
	set str2 "$ba $bb $bc $bd $c"

	puts $f "$str1 $str2"
	flush $f
	close $f
}

RouteGen instproc input-check { } {

	$self instvar check

	set check 1
	$self unicast-check
	$self multicast-check
	$self address-checks
	
	if { $check == -1 } {
		return -1
	}
	return 1
}


RouteGen instproc unicast-check { } {
	$self instvar opts

	# puts "unicast is $opts(unicast)"
	if { $opts(unicast) != "DV" && $opts(unicast) != "Session" } {
	 puts "unknown unicast \"$opts(unicast)\", use DV or Session"
	 $self instvar check 
	 set check -1
	}
}

RouteGen instproc multicast-check { } {
	$self instvar opts

	# puts "multicast is $opts(multicast)"
	if { $opts(multicast) == -1 } {
		puts "multicast will default to \"off\""
	} elseif { $opts(multicast) != "CtrMcast" && \
	 $opts(multicast) != "detailedDM" && $opts(multicast) != "PIM" } {
	 puts "unknown multicast \"$opts(multicast)\", use \
		CtrMcast, detailedDM, or PIM"
	 $self instvar check
	 set check -1
	}
}

RouteGen instproc address-checks { } {
	$self instvar opts

	# puts "expand_address is $opts(expand_address)"

	if { $opts(expand_address) != "on" && \
		$opts(expand_address) != "off" } {
	 puts "address option \"$opts(expand_address)\" unknown"
	 $self instvar check
	 set check -1
	} 
}

