#! /bin/bash
set -e
if [ $# -lt 2 ]; then
  echo "Enter suffix and nsrc";
  exit 5;
fi;

suffix=$1
outdir=$1
nsrc=$2
cd $outdir

num_iters=`ls onramp.$suffix-iter* | wc -l`
num_entries=`cat onramp.$suffix-iter* | wc -l`
if [ $num_entries -ne `expr $num_iters '*' $nsrc` ];
then
  echo "Did not see the right number of entries, only saw $num_entries";
  exit 5
else
  echo "Saw $num_entries, proceeding";
fi;
cat onramp.$suffix-iter*   > onramp.$suffix

num_iters=`ls sfqCoDel.$suffix-iter* | wc -l`
num_entries=`cat sfqCoDel.$suffix-iter* | wc -l`
if [ $num_entries -ne `expr $num_iters '*' $nsrc` ]; then
  echo "Did not see the right number of entries, only saw $num_entries";
  exit 5
else
  echo "Saw $num_entries, proceeding";
fi;
cat sfqCoDel.$suffix-iter* > sfqCoDel.$suffix

num_iters=`ls DropTail.$suffix-iter* | wc -l`
num_entries=`cat DropTail.$suffix-iter* | wc -l`
if [ $num_entries -ne `expr $num_iters '*' $nsrc` ]; then
  echo "Did not see the right number of entries, only saw $num_entries";
  exit 5
else
  echo "Saw $num_entries, proceeding";
fi;
cat DropTail.$suffix-iter* > DropTail.$suffix

cd ..
