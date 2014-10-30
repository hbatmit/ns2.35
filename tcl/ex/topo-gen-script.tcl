# make sure that gt-itm and ns conversion prog are in the path !!!XXX

# until the these two files become part of the release, we
# need to source them
source topo-gen.tcl 
source topo-view.tcl 

# the files that will contain the topology scripts and nam dump files
set topofile mytopo
set topoext tcl
set outfile nam-view-test
set outext nam

set topology_number 1
set hier_flag 0

# create 3 flat random topologies of 12 nodes, with connection 
# probability of 0.25
# for more help on the topology command type "topology -h"
for { set i 0 } { $i < $topology_number } { incr i } {
	topology -outfile $topofile-$i.$topoext -nodes 12 -connection_prob 0.25
#	topology -outfile $topofile-$i.$topoext -type transit_stub -nodes 100 \
 	    -connection_prob 0.1	
}

# after creation, view the topology, the first layout in nam is going
# to be a mess, hit re-layout to get a reasonable layout.
# for now, sgb2ns and sgb2hierns are not combined ; hence pass a flag to view-topology
# 
for { set i 0 } { $i < $topology_number } { incr i } {
	view-topology $topofile-$i.$topoext $outfile-$i.$outext $hier_flag

        puts "Do you want to view another toplogy (Y/N) ?! \[N\]"
        gets stdin x
        if ![regexp {^(y|Y|yes|Yes|YES)} $x] {
                break
        }
}


