#!/bin/sh
# To remove temporary files in tcl/test.
# To run: "./remove.com" or "sh remove.com".
# For the first command, you might have to first make this file executable.
#
rm -f temp* *.ps *core
rm -f t?.tcl ? ecn all packets out drops acks 
rm -f *.tr *.tr1 *.nam *.xgr
rm -f all.*  *.png
rm -f fairflow.* srr_out.txt
rm -f t t? t.*
rm -f chart? 
rm -f flow? flows? 
rm -rf line?
rm -rf Host_A Host_B queue
for i in simple tcp full monitor red sack schedule cbq red-v1 cbq-v1 sack-v1 \
  v1 \
vegas-v1 rbp tcp-init-win tcpVariants ecn manual-routing hier-routing \
intserv webcache mcast newreno srm session mixmode algo-routing vc \
simultaneous lan wireless-lan ecn-ack mip energy wireless-gridkeeper mcache \
satellite wireless-lan-newnode wireless-lan-aodv WLtutorial aimd greis \
rfc793edu friendly rfc2581 links wireless-tdma rio testReno LimTransmit \
pushback diffserv tcp-init-win-full ecn ecn-full simple-full \
red-pd tcpReset LimTransmit pi adaptive-red gk rem vq sack-full \
testReno-full testReno-bayfull source-routing snoop diffusion3 broken \
tcpHighspeed smac quiescent example examples quickstart tcpOptions t \
tagged-trace misc message jobs webtraf sctp xcp smac-multihop \
rng wpan packmime delaybox oddBehaviors wireless-infra
do
	echo test-output-$i
	rm -f test-output-$i/*.test
	rm -f test-output-$i/*[a-y,A-R,T-Z,0-9]
done
# Don't remove *.gz, CVS.



