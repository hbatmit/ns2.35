#! /bin/bash
if [ $# -lt 2 ]; then
  echo "Enter  rtt and address";
  exit
fi
rtt=$1
address=$2
./reduce.sh flowcdf.nconn10
../graphing-scripts/graphmaker flowcdf.nconn10 $rtt
scp flowcdf.nconn10/graphdir/graph-10.svg anirudh@$address:/tmp/result.svg
