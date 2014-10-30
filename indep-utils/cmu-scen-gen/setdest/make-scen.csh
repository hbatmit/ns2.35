#!/bin/csh

###########################################################
# Script for the original version of setdest 
###########################################################


unset noclobber

set outdir = scen_out

set maxspeed = 20 
set numnodes = 50
set maxx = 1500
set maxy = 300
set simtime = 900

foreach pt (0)
foreach scen ( 1 2 3 4 5 6 7 8 9 10)
echo scen $scen : setdest -v 1 -n $numnodes -p $pt -M $maxspeed -t $simtime \
      -x $maxx -y $maxy
time ./setdest -v 1 -n $numnodes -p $pt -M $maxspeed -t $simtime \
      -x $maxx -y $maxy \
      >$outdir/scen-${maxx}x${maxy}-${numnodes}-${pt}-${maxspeed}-${scen}
echo
end
end
