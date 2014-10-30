#! /bin/csh

rm -f varying/flows* varying/tmp* varying/dropRate* varying/data*

ns red-pd.tcl one netMix Varying testUnresp 0 period 1 plotq 0 time 360 verbose 5 > tmp.Off
mv one.netMix.Varying.1.flows flows.Off
awk '{if ($4==10) {bw = 8*($6 - old)/1000000; print $2, bw; old=$6}}' flows.Off > bw.Off
grep curr tmp.Off | awk '{if ($2=="(0)") print $1, $4*100, $6*100}' > dropRate.Off

ns red-pd.tcl one netMix Varying period 1 plotq 0 time 360 verbose 5 > tmp.On
mv one.netMix.Varying.1.flows flows.On
awk '{if ($4==10) {bw = 8*($6 - old)/1000000; print $2, bw; old=$6}}' flows.On > bw.On
grep curr tmp.On | awk '{if ($2=="(0)") print $1, $4*100, $6*100}' > dropRate.On


grep curr tmp.On | awk '{if ($2=="(0)") { sum1+=$11; sum2+=$13}} END {dr=sum1/sum2; frp = sqrt(1.5)/(0.040*sqrt(dr)); sendrate=(8000*frp)/1000000; print 0, sendrate; print 400, sendrate;}' > dropRate.overall

mv flows.* tmp.* bw.* dropRate.* varying
cd varying
gnuplot varying.gp
gv varying.ps &
gv varying_On.ps &
gv varying-dropRate-On.ps &
gv varying-dropRate-Off.ps &

cd ..
