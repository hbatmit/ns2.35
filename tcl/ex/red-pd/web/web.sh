#! /bin/csh

rm -f web/flows* web/tmp* web/dropRate* web/data*

set t1=1000

ns red-pd.tcl one netWeb Web enable 0 time $t1 > tmp0
mv one.netWeb.Web.0.flows flows0
web/web.pl flows0 tmp0 $t1 -

ns red-pd.tcl one netWeb Web time $t1 > tmp1
mv one.netWeb.Web.1.flows flows1
web/web.pl flows1 tmp1 $t1 +

mv tmp? flows? data* web
cd web
gnuplot web.gp
gv web.ps &
gv webBoxes.ps &

cd ..
