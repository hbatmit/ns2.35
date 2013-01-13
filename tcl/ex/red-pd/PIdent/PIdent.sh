#! /bin/csh

rm -f tmp.* flows.* data.*
rm -f PIdent/flows* PIdent/tmp* PIdent/dropRate* PIdent/data*

set t1=500
foreach ftype ( tcp cbr ) 
    echo "Doing ftype $ftype"
    foreach p ( 0.01 0.02 0.03 0.04 0.05 )
	echo "Doing drop ${p}"
	foreach rate ( 0.25 0.50 0.75 1.0 1.25 1.50 1.75 2.00 2.50 3.00 3.50 4.00 ) 
	    echo "Doing rate ${rate}"
	    ns red-pd.tcl one netTestFRp TestIdent p ${p} gamma ${rate} ftype ${ftype} time ${t1} > tmp.${ftype}-$p-$rate
	    mv one.netTestFRp.TestIdent.1.flows flows.${ftype}-${p}-${rate}
	end
	PIdent/PIdent.pl tmp.${ftype}-$p- > data.${ftype}-${p}
    end 
end

mv flows* tmp* data* PIdent
cd PIdent
gnuplot PIdent.gp
gv PIdent_tcp.ps &
gv PIdent_cbr.ps &

cd ..

