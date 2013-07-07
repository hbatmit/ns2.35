#! /bin/bash
killall -s9 ns

if [ $# -lt 11 ]; then
  echo "Enter link_trace, prop. delay, num of users, ontype, avgbytes, cc, offtime, iw, enable_bulk, enable_web, enable_stream";
  exit 5;
fi;

link_trace=$1
onewaydelay=$2
num_srcs=$3
traffic_type=$4
onbytes=$5
tcp_cc=$6
offtime=$7
iw=$8
enable_bulk=$9
enable_web=${10}
enable_stream=${11}

suffix=link.`echo $link_trace | rev | cut -f 1 -d '/' | rev`.$onewaydelay.nconn$num_srcs.$traffic_type.$onbytes.`echo $tcp_cc | rev | cut -d '/' -f1 | rev`.$offtime.iw$iw.bulk$enable_bulk.web$enable_web.stream$enable_stream
i=1
iters=76

outdir=$suffix
rm -rf $outdir
mkdir $outdir

time_constant=0.10

set -x
while [ $i -lt $iters ]; do
  echo $i
#  ../../../ns onramp.tcl -gw SFD      -nsrc $num_srcs -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype $traffic_type -avgbytes $onbytes -iter $i -offavg $offtime -tcp $tcp_cc -cdma_slot 0.00167 -onramp_K $time_constant -droptype time -iw $iw 2> /dev/null | grep sndrttMs > $outdir/onramp.$suffix-iter$i &
  ../../../ns onramp.tcl -gw sfqCoDel -nsrc $num_srcs -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype $traffic_type -avgbytes $onbytes -iter $i -offavg $offtime -tcp $tcp_cc -cdma_slot 0.00167 -iw $iw -enable_bulk $enable_bulk -enable_web $enable_web -enable_stream $enable_stream 2> /dev/null | grep sndrttMs > $outdir/sfqCoDel.$suffix-iter$i &
#  ../../../ns onramp.tcl -gw DropTail -nsrc $num_srcs -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype $traffic_type -avgbytes $onbytes -iter $i -offavg $offtime  -tcp $tcp_cc -cdma_slot 0.00167 -maxq 1000000 -iw $iw 2> /dev/null | grep sndrttMs > $outdir/DropTail.$suffix-iter$i &
  i=`expr $i '+' 1`
done
set +x
