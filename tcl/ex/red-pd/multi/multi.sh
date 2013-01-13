#! /bin/csh

rm -f data* flows*
rm -f multi/flows* multi/tmp* multi/dropRate* multi/data*

set t1=500
foreach ftype ( cbr tcp ) 
	echo "Doing flow type $ftype"
	foreach links ( 1 2 3 4 5 )
		echo "Doing links ${links}"
		ns red-pd.tcl multi netMulti Multi enable 0 links $links ftype $ftype time $t1 
		mv multi.netMulti.Multi.0.flows flows0.${ftype}-${links}
		ns red-pd.tcl multi netMulti Multi links $links ftype $ftype time $t1 
		mv multi.netMulti.Multi.1.flows flows1.${ftype}-${links}
	end
	multi/multi.pl flows0.${ftype}- $t1 0.${ftype}
	multi/multi.pl flows1.${ftype}- $t1 1.${ftype}
end

mv flows* data* multi
cd multi
gnuplot multi.gp
gv multi.ps &

cd ..

