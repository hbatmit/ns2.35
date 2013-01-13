# This prints the Seqnos, and the fraction of packets with that Seqno.
# Input file:
###########################################
# Distribution of Seqnos, 100 seqnos per bins
# 0 to 99: fraction 0.111 number 9384
# 100 to 199: fraction 0.095 number 8101
# 200 to 299: fraction 0.095 number 8021
###########################################
{
if (NR==1) {
   thisbin = 0
   binsize = 0
   maxbin = 1000
}
if ($1=="Distribution"&&$3=="Seqnos,") {
  binsize = $4
  halfbin = binsize/2
}
if (binsize > 0) {
if ($2=="to"&&$4=="seqnos:") {
   bin = $3;
   frac = $6;
   for (i=thisbin; i<bin; i += binsize) {
	aveSeqno = i - halfbin 
	if (aveSeqno < 0) aveSeqno = 0;
	printf "%4.3f 0.0\n", aveSeqno;
   }
   aveSeqno = bin - halfbin;
   if (aveSeqno < 0) aveSeqno = 0;
   printf "%4.3f %5.3f\n", aveSeqno, frac
   thisbin = bin + binsize
}}}
END{
  for (i=thisbin; i<=maxbin; i += binsize) {
     aveSeqno = i - halfbin;
     printf "%4.3f 0.0\n", aveSeqno;
}}
