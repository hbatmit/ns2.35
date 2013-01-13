#! /bin/csh

rm -f response/flows* response/tmp* response/dropRate* response/data*

set t1=360
foreach p ( 0.01 0.02 0.03 )
	echo "Doing p = $p"
	ns red-pd.tcl one netTestFRp Response p ${p} period 1 testUnresp 0 time $t1
	mv one.netTestFRp.Response.1.flows flows1-${p}
	response/response.pl flows1-${p} ${p} 0.040 > data1-${p}
end

set noFlows=5
ns red-pd.tcl one netTestFRp Response p 0.01 flows ${noFlows} period 1 testUnresp 0 time $t1
mv one.netTestFRp.Response.1.flows flowsMulti
response/response.pl flowsMulti 0.01 0.040 > dataMulti

mv flows* data* response
cd response
gnuplot response.gp
gv response.ps &
gv response2.ps &

cd ..
