#! /bin/csh

ns red-pd.tcl one netMix Mix enable 0 verbose -1 plotq 0 time 400
ns red-pd.tcl one netMix Mix verbose -1 plotq 0 time 400

awk '{ if ($4 != 0) { if ($2==190) start[$4]=$6; if ($2==390) bw[$4]=($6 - start[$4])*8/(200*1000000)}} END {for (i=1; i<=12; i++) print i, bw[i]}' one.netMix.Mix.0.flows > data.false

awk '{ if ($4 != 0) { if ($2==190) start[$4]=$6; if ($2==390) bw[$4]=($6 - start[$4])*8/(200*1000000)}} END {for (i=1; i<=12; i++) print i, bw[i]}' one.netMix.Mix.1.flows > data.true

mv one.netMix.Mix* data* mix
cd mix
gnuplot mix.gp
gv mix.ps &

cd ..
