# Created May 98 by Ahmed Helmy; updated June 98
# topology generator class
Class TG

proc usage { } {
	puts stderr {usage: topology [options]

where options are given as: -key value
example options:
-outfile mytopo -type random -nodes 50 -method pure-random

"topology -h" help
}
	return
#	exit 1
}

proc detailed-usage { } {
puts {usage: topology [-<key 1> <value 1> -<key 2> <value 2> -<key n> <value n>]

example options: 
-generator itm -outfile myfile -type random -nodes 20 -connection_prob 0.6 

keys and corresponding values:
-generator
  possible value: itm (default) [the georgia tech topology generator]
  [note: you need to invoke itm and sgb2ns, e.g. by setting your path]

-outfile [the output file that will contain the ns script describing the
	  generated topology. This must be given.]

-type
  possible values: random (default), transit_stub

-seed
  possible values: integer (default = random [uses ns-random])

-nodes [the number of nodes in the topology; used with `-type random']
  possible values: integer (default = 50 with random, 100 with 
  transit-stub)

-scale [used by itm to place nodes in topologies] (default = nodes)

-method [the node connection/linking method; used with `-type random']
  possible values: waxman1, waxman2, pure-random, doar-leslie, exponential, 
                   and  locality (default = pure-random)

-connection_prob [the connection probability between nodes; used in 
       all methods] [this sometimes is called `alpha']
  possible values: 0.0 <= connection_prob <= 1.0 (default = 0.5)

-beta [used only with waxman1, waxman2, doar-leslie and locality]
  possible values: 0.0 <= beta (default = 0.5)

-gamma [used only with doar-leslie and locality]
  possible values: 0.0 <= gamma (default = 0.5)
  }
}

proc itm-random-help { } {
	puts {Comment from ITM, on edge connection methods:
   1. Edge is placed between two nodes by a probabilistic method, which
      is determined by the "method" parameter.  Edge is placed with
      probability p, where p is calculated by one of the methods below,
      using:
        alpha, beta, gamma: input parameters,
        L is scale * sqrt(2.0): the max distance between two points,
        d: the Euclidean distance between two nodes.
        e: a random number uniformly distributed in [0,L]
 
      Method 1: (Waxman's RG2, with alpha,beta)
           p = alpha * exp(-e/L*beta)
      Method 2: (Waxmans's RG1, with alpha,beta)
           p = alpha * exp(-d/L*beta)
      Method 3: (Pure random graph)
           p = alpha
      Method 4: ("EXP" - another distance varying function)
           p = alpha * exp(-d/(L-d))
      Method 5: (Doar-Leslie, with alpha,beta, gamma)
           p = (gamma/n) * alpha * exp(-d/(L*beta))
      Method 6: (Locality with two regions)
           p = alpha     if d <= L*gamma,
           p = beta      if d > L*gamma
 
   2. Constraints
        0.0 <=  alpha  <= 1.0  [alpha is a probability]
        0.0 <= beta            [beta is nonnegative]
        0.0 <= gamma           [gamma is nonnegative]
        n <  scale*scale       [enough room for nodes]
  }
}

proc itm-transit-stub-help { } {
	puts {Parameters for transit_stub topology by itm:

-stubs_per_transit [number of stubs per transit node] (default = 3)
-ts_extra_edges [number of extra transit-stub edges] (default = 0)
-ss_extra_edges [number of extra stub-stub edges] (default = 0) 

-transit_domains [number of transit domains] (default = 1)
-domains_scale [top level scale used by ITM] (default = 20)

* Connectivity of domains [similar to the random topology parameters]
-domains_method (default = pure-random)
-domains_connection_prob (default = 1.0) [fully connected]
-domains_beta (default = 0.5)
-domains_gamma (default = 0.5)

* Connectivity of transit nodes:
-transit_nodes (default = 4)
-transit_scale (default = 20)
-transit_method (default = pure-random)
-transit_connection_prob (default = 0.6)
-transit_beta (default = 0.5)
-transit_gamma (default = 0.5)

* Connectivity of stub nodes:
-stub_nodes (default = 8)
-stub_method (default = pure-random)
-stub_connection_prob (default = 0.4)
-stub_beta (default = 0.5)
-stub_gamma (default = 0.5)

* Total number of nodes is computed as follows:      
 nodes=transit_domains * transit_nodes * (1 + stubs_per_transit * stub_nodes)

 for example, for the above default settings we get:
        1 * 4 ( 1 + 3 * 8 ) = 100 nodes
  }

}

proc help-on-help { } {
	puts {Help available for random, transit stub, and edge connection method.

Help usage "topology -h <i>" 
where: 
<i> = 1 for random, 2 for transit stub, and 3 for edge connection method.
  }
}

proc help { x } {
	switch $x {
		1 { detailed-usage }
		2 { itm-transit-stub-help }
		3 { itm-random-help }
		default { puts "invalid help option"; help-on-help } 
	}
}

proc topology { args } {
	set len [llength $args]

	if $len {
	    set key [lindex $args 0]
            if {$key == "-?" || $key == "--help" || $key == "-help" \
			|| $key == "-h" } {
				if { [set arg2 [lindex $args 1]] == "" } {
					usage
					help-on-help
				} else {
                        	help $arg2
				}
			return
                }
	}

        if [expr $len % 2] {
                # if number is odd => error !
                puts "fewer number of arguments than needed in \"$args\""
                usage
		return
        }

        # default topology generator
        set generator itm

        if { $args != "" && [lindex $args 0] == "-generator" } {
		set generator [lindex $args 1]
		set args [lreplace $args 0 1]
	}

	# check if the generator type exists
	if [catch {set tg [TG/$generator info instances]}] {
		puts "unknown generator type $generator"
		usage
		return
	}
	if { $tg == "" } {
		set tg [new TG/$generator]
	}
	if ![llength $args] {
		$tg create
	} else {
		$tg create $args
	}
	ScenGen setTG $tg
}

Class TG/itm -superclass TG

TG/itm instproc init { } {
	$self next
}

TG/itm instproc default_options { } {
	# default set may not be complete for now.. !XXX
	$self instvar opt_info

	set opt_info {
		# init file to -1, must be supplied by input
		outfile -1

		# number of graphs and seed
		# flat random
		type random

		# number should not be changed by input... should be left 
		# as 1, and a tcl loop may create multiple graphs... left it as 
		# place holder in case this may change later.. !
		number 1
		# seed is randomized later if not entered as input
		seed -1

		nodes 50
		# if not entered assign to nodes later 
		scale -1 
		
		method pure-random
		connection_prob 0.5

		beta 0.5
		gamma 0.5

		# defaults for transit stub
	# total number of nodes is:
	# transit_domains * transit_nodes * (1 + stubs_per_transit * stub_nodes)
	# 1 * 4 ( 1 + 3 * 8 ) = 100 nodes
		stubs_per_transit 3
		ts_extra_edges 0
		ss_extra_edges 0

		transit_domains 1
		domains_scale 20
		domains_method pure-random
		domains_connection_prob 1.0
		domains_beta 0.5
		domains_gamma 0.5

		transit_nodes 4
		transit_scale 20
		transit_method pure-random
		transit_connection_prob 0.6
		transit_beta 0.5
		transit_gamma 0.5

		stub_nodes 8
		# the stub scale is ignored by ITM, is computed as fraction
		# of the transit scale... see proc comment below !
		stub_scale 10
		stub_method pure-random
		stub_connection_prob 0.4
		stub_beta 0.5
		stub_gamma 0.5

		# for N level hierarchy
		# assume all levels use same vars
		levels 3
		level_nodes 10
		level_scale 10
		level_method waxman1
		level_connection_prob 0.7
		level_beta 0.2
		level_gamma 0.5
	}
	$self parse_opts
}
	
TG instproc parse_opts { } {
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

TG instproc parse_input { args } {
	# remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]

	$self instvar opts

	for { set i 0 } { $i < $len } { incr i } {
		set key [lindex $args $i]
		regsub {^-} $key {} key
                if {![info exists opts($key)]} {
			puts stderr "unrecognized option $key"
			usage
			return -1
		}
		incr i
		# puts "changing $key from $opts($key) to [lindex $args $i]"
		set opts($key) [lindex $args $i]
	}
	# puts "end of parsing... "
	return 0
}

TG instproc create { args } {
        # remove the list brackets from the args list
        set args [lindex $args 0]
        set len [llength $args]
        # puts "calling create with args $args, len $len"

	$self default_options

	if $len {
		if { [$self parse_input $args] == -1 } {
			return 
		}
	}
	# check that the filename is provided
	$self instvar opts
	if { $opts(outfile) == -1 } {
puts {you must provide the outfile name !!.. use "topology -h" for help}
		return
	}
	$self create-topology
}

	

# XXX to be extended to include stubs... and other topo info
TG instproc setNodes { type num } {
	$self instvar nodes
	set nodes($type) $num
	# puts "nodes($type) = $nodes($type)"

	# now we stor nodes(all) x
	# should be able to store nodes(stub1) x1-x2
	# keep track of the num of stubs... etc
}

TG instproc getNodes { type } {
	$self instvar nodes
	if { ![info exists nodes($type)]} {
		puts "error: $type doesnot exist! Check srcstub/deststub arguments"
		exit
	}
	return $nodes($type)
}

TG/itm instproc create-topology { } {

	$self instvar opts

	# initialize the seed if not given
	if { $opts(seed) == -1 } {
		set opts(seed) [ns-random]
	}

	puts "type $opts(type), seed $opts(seed)"

	switch $opts(type) {
	  "random" {
		# compose the filename
		set i 0
		while { 1 } {
			# avoid clashing with prev files, incr i
			set topo_filename rand-$opts(nodes)-$i
			if ![file exists $topo_filename] {
				break
			}
			incr i
		}
		set topo_file [open $topo_filename w]

                # write to a file in GA tech format  
                # for now generate 1 graph,... 
		# check the scale, if not give, assign # of nodes
		if { $opts(scale) == -1 } {
			set opts(scale) $opts(nodes)
		}

		$self setNodes all $opts(nodes)

		puts "nodes $opts(nodes), scale $opts(scale), \
		     method $opts(method)"
		puts "conn prob $opts(connection_prob), beta \
		     $opts(beta), gamma $opts(gamma)"

                puts $topo_file "geo $opts(number) $opts(seed)"
		set str [$self rand-line $opts(nodes) $opts(scale) \
		  $opts(method) $opts(connection_prob) $opts(beta) \
		  $opts(gamma)]
		puts $topo_file $str
	  }
	  "transit_stub" {
		# filename
                set i 0
                while { 1 } {
                        # avoid clashing with prev files, incr i
	set topo_filename ts-$opts(transit_domains)-$opts(transit_nodes)-$i
                        if ![file exists $topo_filename] {
                                break
                        }
                        incr i   
                }
                set topo_file [open $topo_filename w]  
              
		# debugging stuff !!!
		puts "stubs per transit $opts(stubs_per_transit), \
		 ts extra $opts(ts_extra_edges), ss extra $opts(ss_extra_edges)"
		puts "transit domains $opts(transit_domains), \
		 domains_scale $opts(domains_scale), domains_method \
		 $opts(domains_method)"
		puts "domains_conn $opts(domains_connection_prob), \
			beta $opts(domains_beta), gamma $opts(domains_gamma)"
		puts "transit_nodes $opts(transit_nodes), \
		  transit_scale $opts(transit_scale), \
		  transit_method $opts(transit_method)"
		puts "transit prob $opts(transit_connection_prob) \
		  transit_beta $opts(transit_beta), \
		  transit_gamma $opts(transit_gamma)"
		puts "stub_nodes $opts(stub_nodes), \
		  stub_method $opts(stub_method), \
		  stub_conn $opts(stub_connection_prob) \
		  stub_beta $opts(stub_beta) \
		  stub_gamma $opts(stub_gamma)"

		set total_nodes [expr $opts(transit_domains) * \
		  $opts(transit_nodes) * [expr 1 + $opts(stubs_per_transit) \
		  * $opts(stub_nodes)]]
		puts "Total nodes = $total_nodes\n"

		#check if number of nodes specified is diff from
		# the total nodes after computation.
		if {$opts(nodes) != $total_nodes} {
			puts "\n ERROR: Nodes specified = $opts(nodes)                        \nActual number of total nodes = $total_nodes
			\nFor transit_stub type, specify value of \"transit_domains\", \"transit_nodes\", \"stubs_per_transit\" and \"stub_nodes\" to change the number of nodes in your topology\n"
			
			exit 1
			
		}	
		$self setNodes all $total_nodes

		puts $topo_file "ts $opts(number) $opts(seed)"
		puts $topo_file "$opts(stubs_per_transit) \
			$opts(ts_extra_edges) $opts(ss_extra_edges)"
		set str [$self rand-line $opts(transit_domains) \
		  $opts(domains_scale) $opts(domains_method) \
		  $opts(domains_connection_prob) $opts(domains_beta) \
		  $opts(domains_gamma)]
		puts $topo_file $str
		set str [$self rand-line $opts(transit_nodes) \
		  $opts(transit_scale) $opts(transit_method) \
		  $opts(transit_connection_prob) $opts(transit_beta) \
		  $opts(transit_gamma)]
		puts $topo_file $str
		set str [$self rand-line $opts(stub_nodes) \
		  $opts(stub_scale) $opts(stub_method) \
		  $opts(stub_connection_prob) $opts(stub_beta) \
		  $opts(stub_gamma)]
		puts $topo_file $str
	  }
	  default { 
	   puts "invalid type \"$opts(type)\"!! use \"topology -h\" for help"
	   return
	  }
	}
	flush $topo_file
	close $topo_file
        $self generate-topo-gatech $topo_filename
	# cleanup if you want.. uncomment the next line !
	# exec rm $topo_filename
	# call the converter 'number' times
	# no need for this loop if number is always 1 .. XXX
	for { set i 0 } { $i < $opts(number) } { incr i } {
        	# output file is appended by -i.gb 
        	$self convert-to-ns $topo_filename-$i.gb $opts(outfile) $opts(type)

	}
	# clean up... not sure if the file is needed by hierarchical addressing
	# exec rm $topo_filename-0.gb

}

TG/itm instproc rand-line { nodes scale method conn_prob beta gamma } {
	lappend str $nodes $scale
	switch $method {
                "waxman1" {
                  # need alpha and beta
                  lappend str 1 $conn_prob $beta
                }
                "waxman2" {
                  lappend str 2 $conn_prob $beta
                }
                "pure-random" {
                  # needs only alpha
                  lappend str 3 $conn_prob
                }
                "doar-leslie" {
                  # alpha, beta and gamma... X seems to use only alpha !
                  lappend str 4 $conn_prob $beta $gamma
                } 
                "exponential" {
                   # alpha .. XXX doesn't work now !!
                  lappend str 5 $conn_prob
		}
                "locality" {
		   # alpha , beta and gamma
		   lappend str 6 $conn_prob $beta $gamma
		}
                default {
                        puts "unidentified method $method .. aborting !!"
			usage
                        return
                }
	}
	# puts "str $str"
	return $str
}

TG/itm instproc generate-topo-gatech { fn } {
        exec itm $fn
}
                   
TG/itm instproc convert-to-ns { fn outfile type} {
	
	# to avoid generating false errors if sgb2ns is careless
	# about its return/exit value
	#
	# topofile generated by sgb2hierns program; has topology info used by
	# topogen and agentgen
	# So If type permits, read in topofile and cleanup.
	#
	if {$type == "transit_stub"} {
		set topofile $outfile.topoinfo
		catch { exec sgb2hierns $fn $outfile $topofile }
		$self getTopoInfo $topofile
		#exec rm $topofile
		return
	} 
	catch { exec sgb2ns $fn $outfile}
}

#
# Read in topology info from topo.$outfile
#
TG/itm instproc getTopoInfo topofile {
	set input [open $topofile r]
	foreach line [split [read $input] \n] {
		set stream [split $line]
		switch [lindex $stream 0] {
			"transits" {$self getLine $stream "transit"}
			"total-stubs" {$self setNodes "total-stubs" [lindex $stream 1]}
			"stubs" {$self getLine $line "stub"}
			"nodes/stub" {$self getLine $line "nodes/stub"}
		}
	}
	close $input
}


#
# Read each line and store away topoinfo
#

TG/itm instproc getLine {line type} {
	$self instvar nodes
	for {set i 1} {$i < [llength $line]} {incr i} {
		if {$type == "nodes/stub"} {
			set start $nodes(stub$i)
			set range [lindex $line $i]
			# puts "start=$start range=$range"
			$self setNodes stub$i $start:[expr $start + $range - 1]
		} else {
			$self setNodes $type$i [lindex $line $i]
		}
	}
}






proc comment { } {
puts {
 * itm.c -- Driver to create graphs using geo(), geo_hier(), and transtub().
 *
 * Each argument is a file containing ONE set of specs for graph generation.
 * Such a file has the following format:
 *    <method keyword> <number of graphs> [<initial seed>]
 *    <method-dependent parameter lines>
 * Supported method names are "geo", "hier", and "ts".
 * Method-dependent parameter lines are described by the following:
 *    <geo_parms> ::=
 *        <n> <scale> <edgeprobmethod> <alpha> [<beta>] [<gamma>]
 *    <"geo" parms> ::= <geo_parms>
 *    <"hier" parms> ::=
 *          <number of levels> <edgeconnmethod> <threshold>
 *          <geo_parms>+  {one per number of levels}
 *    <"ts" parms> ::=
 *          <# stubs/trans node> <#t-s edges> <#s-s edges>
 *          <geo_parms>       {top-level parameters}
 *          <geo_parms>       {transit domain parameters}
 *          <geo_parms>       {stub domain parameters}
 *
 * Note that the stub domain "scale" parameter is ignored; a fraction
 * of the transit scale range is used.  This fraction is STUB_OFF_FACTOR,
 * defined in ts.c.
 *
 * From the foregoing, it follows that best results will be obtained with
 *   -- a SMALL value for top-level scale parameter
 *   -- a LARGE value for transit-level scale parameter
 * and then the value of stub-level scale parameter won't matter.
 *
 * The indicated number of graphs is produced using the given parameters.
 * If the initial seed is present, it is used; otherwise, DFLTSEED is used.
 *
 * OUTPUT FILE NAMING CONVENTION:
 * The i'th graph created with the parameters from file "arg" is placed
 * in file "arg-i.gb", where the first value of i is zero.
 }
}

###############################################################
### the scenario generator keeps track of topology generator ##
### and can be queried to get this info			     ##
###############################################################

Class ScenGen

ScenGen set TG ""

ScenGen proc setTG { tg } {
	ScenGen set TG $tg
}

ScenGen proc getTG { } {
	return [ScenGen set TG]
}
