#!/bin/csh

#######################################################
# Script for the modified version of setdest
#######################################################


unset noclobber

#######################################################
#  Speed Selections 
#   1. uniform speed [min, max]
#   2. normal speed [min, max]
#######################################################
set speedsel = 1 	 	
set minspeed = 1 
set maxspeed = 19 

#######################################################
#  Pause Selections
#   1. constant  
#   2. uniform [0, 2 x pause]
#######################################################
set pausesel = 1 
set pause = 50

set outdir = scen_out_ss

set numnodes = 50
set maxx = 1500
set maxy = 300
set simtime = 900 

foreach scen ( 1 2 3 4 5 6 7 8 9 10)
echo scen $scen : setdest -v 2 -n $numnodes -s $speedsel -m $minspeed -M $maxspeed \
      -t $simtime -P $pausesel -p $pause -x $maxx -y $maxy
time ./setdest -v 2 -n $numnodes -s $speedsel -m $minspeed -M $maxspeed -t $simtime \
      -P $pausesel -p $pause -x $maxx -y $maxy \
      >$outdir/scen-s${speedsel}-${maxx}x${maxy}-n${numnodes}-m${minspeed}-M${maxspeed}-p${pause}-${scen}
echo
end
