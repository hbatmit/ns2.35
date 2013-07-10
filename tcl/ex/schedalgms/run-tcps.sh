#! /bin/bash
killall -s9 ns

if [ $# -lt 16 ]; then
  echo "Enter downlink_trace, uplink_trace, prop. delay, num of users, ontype, avgbytes, cc, offtime, iw, enable_bulk, enable_web, enable_stream, maxq, num_classes, bt_target, web_target";
  exit 5;
fi;

downlink_trace=$1
uplink_trace=$2
onewaydelay=$3
num_srcs=$4
traffic_type=$5
onbytes=$6
tcp_cc=$7
offtime=$8
iw=$9
enable_bulk=${10}
enable_web=${11}
enable_stream=${12}
maxq=${13}
num_classes=${14}
bt_target=${15}
web_target=${16}

suffix=link.dl`echo $downlink_trace | rev | cut -f 1 -d '/' | rev`.ul`echo $uplink_trace | rev | cut -f 1 -d '/' | rev`.$onewaydelay.nconn$num_srcs.$traffic_type.$onbytes.`echo $tcp_cc | rev | cut -d '/' -f1 | rev`.$offtime.iw$iw.bulk$enable_bulk.web$enable_web.stream$enable_stream.maxq$maxq.num_classes$num_classes.bt_target$bt_target.web_target$web_target
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
  ../../../ns onramp.tcl -gw sfqCoDel -nsrc $num_srcs -simtime 100 -sched pf -downlink $downlink_trace -uplink $uplink_trace -delay $onewaydelay -ontype $traffic_type -avgbytes $onbytes -iter $i -offavg $offtime -tcp $tcp_cc -cdma_slot 0.00167 -iw $iw -enable_bulk $enable_bulk -enable_web $enable_web -enable_stream $enable_stream -maxq $maxq -num_classes $num_classes -bt_target $bt_target -web_target $web_target -verbose true 2> /dev/null > $outdir/sfqCoDel.$suffix-iter$i &
#  ../../../ns onramp.tcl -gw DropTail -nsrc $num_srcs -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype $traffic_type -avgbytes $onbytes -iter $i -offavg $offtime  -tcp $tcp_cc -cdma_slot 0.00167 -maxq 1000000 -iw $iw 2> /dev/null | grep sndrttMs > $outdir/DropTail.$suffix-iter$i &
  i=`expr $i '+' 1`
done
set +x
