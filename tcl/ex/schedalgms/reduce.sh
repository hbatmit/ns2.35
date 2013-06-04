#! /bin/bash
suffix=$1
outdir=$1
cd $outdir
cat onramp100.$suffix-iter* > onramp100.$suffix
cat sfqCoDel.$suffix-iter* > sfqCoDel.$suffix
rm *iter*
