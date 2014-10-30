#! /bin/csh

rm -f data* flows*
rm -f pktsVsBytes/flows* pktsVsBytes/tmp* pktsVsBytes/dropRate* 

set t1=1000
foreach try ( 2 )
    echo "Doing iteration $try, for RED in packet mode."
    ## This uses topology netPktsVsBytes1 for packet mode.
    foreach flows ( 4 8 12 16 20 )
	    echo "Doing flows ${flows}"
	    ns red-pd.tcl one netPktsVsBytes1 PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1 enable 0
	    mv one.netPktsVsBytes1.PktsVsBytes.0.flows flows0-${try}-${flows}

	    ns red-pd.tcl one netPktsVsBytes1 PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1
	    mv one.netPktsVsBytes1.PktsVsBytes.1.flows flows1-${try}-${flows}

      end
      pktsVsBytes/pktsVsBytesA.pl flows0 $t1 $try 0
      pktsVsBytes/pktsVsBytesA.pl flows1 $t1 $try 1
end

foreach try ( 1 )
    echo "Doing iteration $try, for RED in byte mode."
    foreach flows ( 4 8 12 16 20 )
	    echo "Doing flows ${flows}"
	    ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1 enable 0
	    mv one.netPktsVsBytes.PktsVsBytes.0.flows flows0-${try}-${flows}

            ns red-pd.tcl one netPktsVsBytes PktsVsBytes flows ${flows} testUnresp 0 verbose -1 time $t1
	    mv one.netPktsVsBytes.PktsVsBytes.1.flows flows1-${try}-${flows}

    end
    pktsVsBytes/pktsVsBytesA.pl flows0 $t1 $try 0
    pktsVsBytes/pktsVsBytesA.pl flows1 $t1 $try 1
end

mv flows* data* pktsVsBytes
cd pktsVsBytes
gnuplot pktsVsBytesA.gp
gv pktsVsBytes0-A.ps &
gv pktsVsBytes1-A.ps &
gv pktsVsBytes0.ps &
gv pktsVsBytes1.ps &

cd ..

