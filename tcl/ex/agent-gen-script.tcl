# This file is an example script for using the topology generator,
# the routing generator and the agents generator together as part
# of the scenario generator
# Created by Ahmed Helmy, June 98.

# source the generators code (this is not part of the release yet)
source topo-gen.tcl
source agent-gen.tcl
source route-gen.tcl

# setup the files that will contain the scripts generated
set topofile topo1.tcl
set agentfile agent1.tcl
set routefile route1.tcl

set hier_flag 1

# we remove the agentfile, to start afresh... the agentfile is
# opened in append mode and previous runs may cause problems
if { [file exists $agentfile] } {
	catch { [exec rm $agentfile] }
}

# generate the topology creation script
#puts {topology -outfile $topofile -num 50 -connection_prob 0.1}
#topology -outfile $topofile -nodes 15 -connection_prob 0.1

puts {topology -outfile $topofile -type transit_stub -nodes 50 -connection_prob 0.1}
topology -outfile $topofile -type transit_stub -nodes 50 -connection_prob 0.1

# set the route settings for unicast/mcast
puts {routing -outfile $routefile -unicast Session -multicast CtrMcast}
routing -outfile $routefile -unicast Session -multicast CtrMcast

# generate the agent creation script
# note that src and dest stub location info (for distribution of src and dest nodes)
# should be given only when topology generated is of type transit-stub

puts {agents -outfile $agentfile -transport TCP/Reno -src FTP \
	-sink TCPSink/DelAck -num 60% -srcstub 1-4,8,10 -deststub 5-7,9,11,12 -start 1-3 -stop 9-11}
agents -outfile $agentfile -transport TCP/Reno -src FTP \
    -sink TCPSink/DelAck -num 60% -srcstub 1-4,8,10 -deststub 5-7,9,11,12 -start 1-3 -stop 9-11

#puts {agents -outfile $agentfile -transport TCP -sink TCPSink -num 8}
#agents -outfile $agentfile -transport TCP -sink TCPSink -num 8

#puts {agents -outfile $agentfile -transport SRM/Deterministic -num 75% \
	-join 0-2 -src CBR/UDP -traffic Pareto -srcnum 20% \
	-start 3-10}
#agents -outfile $agentfile -transport SRM/Deterministic -num 75% \
	-join 0-2 -src CBR/UDP -traffic Pareto -srcnum 20% \
	-start 3-10 

#puts {agents -outfile $agentfile -transport SRM -src CBR/UDP \
	-traffic Expoo }
#agents -outfile $agentfile -transport SRM -src CBR/UDP \
	-traffic Expoo

mark-end $agentfile
# the marker merely serves as debugging aid to make sure that
# all the code was written to file, and was not disrupted due
# to failure or disk full... etc, it also contains all the 
# the command lines used to generate the code.

# now that we've generated the tcl scripts, source them and call
# the procs to start the simulations.
source $routefile
source $topofile
source $agentfile

set ns [new Simulator]

# set up hierarchical routing (if needed) or any other address format
#$ns set-address-format hierarchical

# comment out the ns trace for now... it consumes a lot of mem
# set f [open out.tr w]
# $ns trace-all $f
set nf [open out.nam w]
$ns namtrace-all $nf

# have to call these before the topology is generated, as
# artifact of current ns !!!
setup-mcastNaddr ns

# call the topology script
if {$hier_flag} {
	create-hier-topology ns node 1.5Mb
} else {
	create-topology ns node 1.5Mb
}

# set routing
create-routing ns

# call the agent script
generate-agents ns node

$ns at 3.0 "finish"

proc finish {} {
	global ns nf; #f
	$ns flush-trace
	# close $f
	close $nf

	puts "running nam..."
	exec nam out.nam &
	exit 0
}

$ns run
