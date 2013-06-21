#! /bin/bash
if [ $# -lt 1 ]; then
  echo "Enter  rtt";
  exit
fi
rtt=$1
./reduce.sh flowcdf.nconn10
../graphing-scripts/graphmaker flowcdf.nconn10 $rtt
scp flowcdf.nconn10/graphdir/graph-10.svg anirudh@anirudh.csail.mit.edu:/tmp/result.svg
