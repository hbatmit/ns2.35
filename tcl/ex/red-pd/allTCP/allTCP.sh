#! /bin/csh

rm -f dropRate data 
rm -f allTCP/flows* allTCP/tmp* allTCP/dropRate* allTCP/data*

set t1=500
foreach rtt ( 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.100 0.110 0.120 0.130 0.140 0.150 0.160 0.170 )
	echo "Doing rtt ${rtt}"
	ns red-pd.tcl one netAllTCP AllTCP rtt ${rtt} verbose 5 time $t1 > tmp-${rtt}
	mv one.netAllTCP.AllTCP.1.flows flows-${rtt}
    end

echo "Doing Disabled"
ns red-pd.tcl one netAllTCP AllTCP enable 0 verbose -1 time $t1 > tmp-disabled
mv one.netAllTCP.AllTCP.0.flows flows-disabled
allTCP/allTCP.disabled.pl flows-disabled $t1 10 170 10

allTCP/allTCP.pl flows-0 $t1
mv flows-* tmp-* dropRate data data.disabled allTCP
cd allTCP
gnuplot allTCP.gp
gv allTCP.ps &
gv allTCP-dropRate.ps &

cd ..

