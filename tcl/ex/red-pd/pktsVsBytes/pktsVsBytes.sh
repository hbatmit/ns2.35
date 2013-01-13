#! /bin/csh

rm -f data* flows*
rm -f pktsVsBytes/flows* pktsVsBytes/tmp* pktsVsBytes/dropRate* pktsVsBytes/data*

set t1=1000
foreach try ( 1 )
    echo "Doing iteration $try"
    foreach flows ( 4 8 12 16 20 )
	    echo "Doing flows ${flows}"
	    ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1 enable 0
	    mv one.netPktsVsBytes.PktsVsBytes.0.flows flows0-${try}-${flows}

	    ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1
	    mv one.netPktsVsBytes.PktsVsBytes.1.flows flows1-${try}-${flows}

end

pktsVsBytes/pktsVsBytes.pl flows0 $t1 1 0
pktsVsBytes/pktsVsBytes.pl flows1 $t1 1 1

mv flows* data* pktsVsBytes
cd pktsVsBytes
gnuplot pktsVsBytes.gp
gv pktsVsBytes0.ps &
gv pktsVsBytes1.ps &

cd ..

