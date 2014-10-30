#!/usr/bin/awk -f
# convert from raw2xg output into gnuplot format
BEGIN {system("rm -f acks packets drops")}
{if (ack) print $1 " " $2 >>"acks"}
{if (pkt) print $1 " " $2 >>"packets"}
{if (drp) print $1 " " $2 >>"drops"}
$1 ~ /^.ack/ {ack=1}
$1 ~ /^.packets/ {pkt=1}
$1 ~ /^.drops/ {drp=1}
$1 ~ /^$/ {ack=0; pkt=0; drp=0}

