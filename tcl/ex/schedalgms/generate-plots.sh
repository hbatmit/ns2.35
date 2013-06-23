#! /bin/bash
if [ $# -lt 4 ]; then
  echo "Enter  rtt, suffix, address, and nsrc";
  exit
fi
rtt=$1
suffix=$2
address=$3
nsrc=$4
./reduce.sh $suffix
../graphing-scripts/graphmaker $suffix $rtt
scp $suffix/graphdir/graph-$nsrc.svg anirudh@$address:/tmp/result$suffix.svg
