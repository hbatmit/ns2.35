#!/bin/sh
# Run all simulations for the technical report on modeling wireless links

if [ -z $1 ]; then
 GP="off"; QUIET=1
else 
  if [ $1 = "gp" ]; then
     GP="on"; QUIET=1 
  fi
  if [ $1 = "xgraph" ]; then
     GP="off"; QUIET=0
  fi
  if [ $1 = "quiet" ]; then
     GP="off"; QUIET=1
  fi
fi

NS=${NS:-../../../ns}

# GPRS good
$NS mtp.tcl -type gprs \
-allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' \
-allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' -quiet $QUIET
./timeseq.cmd tr-gprs-good
if [ $GP = "on" ]; then gv tr-gprs-good.eps; fi

# GPRS mediocre
# delayInt and delayLen have to be on the same line, why?
$NS mtp.tcl -type gprs \
-allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' \
-allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' \
-delayInt 'E(0.1)' -delayLen 'E(0.1)' \
-quiet $QUIET
./timeseq.cmd tr-gprs-medioc
if [ $GP = "on" ]; then gv tr-gprs-medioc.eps; fi

# GPRS poor
$NS mtp.tcl -type gprs \
-allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' \
-allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' \
-delayInt 'E(0.3)' -delayLen 'E(0.3)' \
-errRateUL 0.01 -errBurstUL 0.3 -errSlotUL 3 \
-errRateDL 0.01 -errBurstDL 0.3 -errSlotDL 3 \
-quiet $QUIET
./timeseq.cmd tr-gprs-poor
if [ $GP = "on" ]; then gv tr-gprs-poor.eps; fi

# GPRS mobility
$NS mtp.tcl -type gprs \
-allocLenDL 'U(0.16,0.19)' \
-allocHoldDL 'U(2,5)' \
-allocLenUL 'U(0.5,0.6)' \
-allocHoldUL 'U(0.01,0.4)' \
-vhoTarget gprs -vhoDelay 5 \
-vhoLoss 0.5 -quiet $QUIET
./timeseq.cmd tr-gprs-mobile
if [ $GP = "on" ]; then gv tr-gprs-mobile.eps; fi

# UMTS
$NS mtp.tcl -type umts \
-delayInt 'E(0.1)' \
-delayLen 'E(0.04)' -quiet $QUIET -stop 50
./timeseq.cmd tr-umts
if [ $GP = "on" ]; then gv tr-umts.eps; fi

# UMTS with bandwidth oscillation
$NS mtp.tcl -type umts \
-delayInt 'E(0.1)' \
-delayLen 'E(0.04)' \
-bwLowLen 5 \
-bwHighLen 1 \
-bwScale 10 \
-quiet $QUIET -stop 50
./timeseq.cmd tr-umts-oscil
if [ $GP = "on" ]; then gv tr-umts-oscil.eps; fi

# WLAN in good radio conditions
$NS mtp.tcl -type wlan_complex -bwDL 1Mb -quiet $QUIET -stop 30
./timeseq.cmd tr-wlan 
if [ $GP = "on" ]; then gv tr-wlan.eps; fi

# WLAN in mediocre conditions
$NS mtp.tcl -type wlan_complex -nodeDist 250 -quiet $QUIET -stop 30
./timeseq.cmd tr-wlan-medioc 
if [ $GP = "on" ]; then gv tr-wlan-medioc.eps; fi

# GEO in good radio conditions
$NS mtp.tcl -type geo -quiet $QUIET -stop 30
./timeseq.cmd tr-geo
if [ $GP = "on" ]; then gv tr-geo.eps; fi

# GEO with reordering
$NS mtp.tcl -type geo -reorderLen 0.5 -reorderRate 0.01 \
-quiet $QUIET -stop 30
./timeseq.cmd tr-geo-reord
if [ $GP = "on" ]; then gv tr-geo-reord.eps; fi


