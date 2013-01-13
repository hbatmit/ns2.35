# This prints the RTT in seconds, and the fraction of packets with that RTT.
# Input file:
###########################################
# Distribution of RTTs, 10 ms bins
# 0 to 10 ms: fraction 0.003 number 488
# 160 to 170 ms: fraction 0.000 number 80
###########################################
{
if (NR==1) {
   thisbin = 0;
   binsize = 0;
   maxbin = 200;
}
if ($1=="Distribution"&&$3=="RTTs,") {
  binsize = $4;
  halfbin = binsize / 2;
}
if (binsize > 0) {
  if ($2=="to"&&$4=="ms:") {
     bin = $3;
     frac = $6;
     if (bin > 0) {
       for (i=thisbin; i<bin; i += binsize) {
    	 avertt = (i-halfbin)/1000; 
    	 if (avertt < 0) avertt = 0;
    	 printf "%4.3f 0.0\n", avertt;
       }
       avertt = (bin - halfbin)/1000;
       if (avertt < 0) avertt = 0;
       printf "%4.3f %5.3f\n", avertt, frac;
       thisbin = bin + binsize;
    }
  }
}
}
END{
  for (i=thisbin; i<=maxbin; i += binsize) {
     avertt = (i-halfbin)/1000;
     printf "%4.3f 0.0\n", avertt;
}}
