#! /bin/bash
suffix=$1
outdir=$1
cd $outdir
cat onramp.$suffix-iter*   > onramp.$suffix
cat sfqCoDel.$suffix-iter* > sfqCoDel.$suffix
cat DropTail.$suffix-iter* > DropTail.$suffix
rm *iter*
