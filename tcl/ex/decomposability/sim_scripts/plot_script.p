set term png size 1920,1080
set macros
formatting = "lw 4 w linespoints"
set key bottom left
set output "delay-agility.png"
set title  "Delay Agility"
set xlabel "RTT in ms"
set ylabel "Normalized utility"
ordinate = "log($2/31.622) - log($3 / (2 * $1 + 2))"
plot "delay1000x.plot" u (2 * $1):(@ordinate) @formatting, \
     "delay100x.plot"  u (2 * $1):(@ordinate) @formatting, \
     "delay10x.plot"   u (2 * $1):(@ordinate) @formatting, \
     "delaycubic.plot" u (2 * $1):(@ordinate) @formatting, \
     "delaycubicsfqCoDel.plot" u (2 * $1):(@ordinate) @formatting

set output "workload-agility.png"
set title  "Workload Agility"
set xlabel "On percentage"
set ylabel "Normalized utility"
ordinate ="log($2/31.622) - log($3/150)"
plot "dutycycle1000x.plot" u 1:(@ordinate) @formatting, \
     "dutycycle100x.plot"  u 1:(@ordinate) @formatting, \
     "dutycycle10x.plot"   u 1:(@ordinate) @formatting, \
     "dutycyclecubic.plot" u 1:(@ordinate) @formatting, \
     "dutycyclecubicsfqCoDel.plot" u 1:(@ordinate) @formatting

set output "link-agility.png"
set title  "Link Speed Agility"
set xlabel "Link speed"
set ylabel "Normalized utility"
set logscale x
ordinate ="log($2/$1) - log($3/150)"
plot "linkspeed1000x.plot" u 1:(@ordinate) @formatting, \
     "linkspeed100x.plot"  u 1:(@ordinate) @formatting, \
     "linkspeed10x.plot"   u 1:(@ordinate) @formatting, \
     "linkspeedcubic.plot" u 1:(@ordinate) @formatting, \
     "linkspeedcubicsfqCoDel.plot" u 1:(@ordinate) @formatting

set output "muxing-agility.png"
set title  "Multiplexing Agility"
set xlabel "Number of senders"
set ylabel "Normalized utility"
unset logscale x
ordinate ="log($2/(31.622/$1)) - log($3/150)"
plot "num_senders1000x.plot" u 1:(@ordinate) @formatting, \
     "num_senders100x.plot"  u 1:(@ordinate) @formatting, \
     "num_senders10x.plot"   u 1:(@ordinate) @formatting, \
     "num_senderscubic.plot" u 1:(@ordinate) @formatting, \
     "num_senderscubicsfqCoDel.plot" u 1:(@ordinate) @formatting
