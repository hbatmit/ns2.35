#! /bin/csh

rm -f flow* tmp* dropRate* data*
rm -f tfrc/flows* tfrc/tmp* tfrc/dropRate* tfrc/data*


set t1=500
foreach try ( 1 2 3 4 5 )
	echo "Doing iteration $try"
	foreach flows ( 8 12 16 20 24 28 32 )
		echo "Doing flows ${flows}"
		ns red-pd.tcl one netTFRC TFRC enable 0 rtt 0.060 flows ${flows} verbose 1 time $t1 > tmp0-${try}-${flows}
		mv one.netTFRC.TFRC.0.flows flows0-${try}-${flows}
		ns red-pd.tcl one netTFRC TFRC rtt 0.060 flows ${flows} verbose 1 time $t1 > tmp1-${try}-${flows}
		mv one.netTFRC.TFRC.1.flows flows1-${try}-${flows}
	end
end

tfrc/tfrc.pl flows0 0 $t1 5
tfrc/tfrc.pl flows1 1 $t1 5

mv flows* tmp* data* dropRate* tfrc
cd tfrc
gnuplot tfrc.gp
gv tfrcLow.ps &
gv tfrcHigh.ps &
gv tfrc.ps &
gv tfrc-dropRate.ps &

cd ..

