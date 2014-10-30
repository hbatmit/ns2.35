#! /bin/csh


ns red-pd.tcl one netAllUDP AllUDP enable 0 verbose -1 plotq 0 time 400
ns red-pd.tcl one netAllUDP AllUDP testUnresp 0 verbose -1 plotq 0 time 400

awk '{ if ($4 != 0) { if ($2==190) start[$4]=$6; if ($2==390) bw[$4]=($6 - start[$4])*8/(200*1000000)}} END {for (i=1; i<=11; i++) print i, bw[i]}' one.netAllUDP.AllUDP.0.flows > data.false

awk '{ if ($4 != 0) { if ($2==190) start[$4]=$6; if ($2==390) bw[$4]=($6 - start[$4])*8/(200*1000000)}} END {for (i=1; i<=11; i++) print i, bw[i]}' one.netAllUDP.AllUDP.1.flows > data.true

mv one.netAllUDP* data* allUDP
cd allUDP
gnuplot allUDP.gp
gv allUDP.ps &

cd ..
