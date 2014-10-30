#! /bin/csh

rm -f singleVsMulti/flows* singleVsMulti/tmp* singleVsMulti/dropRate* singleVsMulti/data*

set t1=2000
foreach rtt ( 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.10 0.11 0.12 )
	echo "Doing rtt ${rtt}"
	ns red-pd.tcl one netSingleVsMulti TestIdent testRTT ${rtt} ftype tcp time $t1 > tmp-${rtt}
	mv one.netSingleVsMulti.TestIdent.1.flows flows-${rtt}
end

singleVsMulti/singleVsMulti.pl tmp- 

mv flows-* tmp* data-* singleVsMulti
cd singleVsMulti
gnuplot singleVsMulti.gp
gv singleVsMulti.ps &

cd ..

