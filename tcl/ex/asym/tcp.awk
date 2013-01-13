BEGIN {
	if (dir == "")
		dir = ".";
}	

{
time = $2;
saddr = $4;
sport = $6;
daddr = $8;
dport = $10;
hiack = $14;
cwnd = $18;
ssthresh = $20;
srtt = $26;
rttvar = $28;
ownd = $34;
owndcorr = $36;
nrexmit = $38;

if (!((saddr, sport, daddr, dport) in starttime)) {
	starttime[saddr, sport, daddr, dport] = time;
	ind[saddr, sport, daddr, dport] = sprintf("%s,%s-%s,%s", saddr, sport, daddr, dport);
}
if ((cwnd >= 100) && (!((saddr, sport, daddr, dport) in cwndtime))) {
	cwndtime[saddr, sport, daddr, dport] = time;
	cwndhiack[saddr, sport, daddr, dport] = hiack;
}

if ((time >= mid) && (!((saddr, sport, daddr, dport) in middletime))) {
	middletime[saddr, sport, daddr, dport] = time;
	middlehiack[saddr, sport, daddr, dport] = hiack;
}

if ((time >= turnon) && (!((saddr, sport, daddr, dport) in turnontime))) {
	turnontime[saddr, sport, daddr, dport] = time;
	turnonhiack[saddr, sport, daddr, dport] = hiack;
}

if (time <= turnoff) {
	turnofftime[saddr, sport, daddr, dport] = time;
	turnoffhiack[saddr, sport, daddr, dport] = hiack;
	numrexmit[saddr, sport, daddr, dport] = nrexmit;
}

endtime[saddr, sport, daddr, dport] = time;
highest_ack[saddr, sport, daddr, dport] = hiack;


if (!((saddr, sport, daddr, dport) in cwndfile)) {
	cwndfile[saddr, sport, daddr, dport] = sprintf("%s/cwnd-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > cwndfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > cwndfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in ssthreshfile)) {
	ssthreshfile[saddr, sport, daddr, dport] = sprintf("%s/ssthresh-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > ssthreshfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > ssthreshfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in owndfile)) {
	owndfile[saddr, sport, daddr, dport] = sprintf("%s/ownd-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > owndfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > owndfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in owndcorrfile)) {
	owndcorrfile[saddr, sport, daddr, dport] = sprintf("%s/owndcorr-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > owndcorrfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > owndcorrfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in srttfile)) {
	srttfile[saddr, sport, daddr, dport] = sprintf("%s/srtt-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > srttfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > srttfile[saddr,sport,daddr,dport];
}
if (!((saddr, sport, daddr, dport) in rttvarfile)) {
	rttvarfile[saddr, sport, daddr, dport] = sprintf("%s/rttvar-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
	printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > rttvarfile[saddr,sport,daddr,dport];
	printf "Device: Postscript\n" > rttvarfile[saddr,sport,daddr,dport];
}

printf "%g %g\n", time, cwnd > cwndfile[saddr, sport, daddr, dport];
printf "%g %d\n", time, ssthresh > ssthreshfile[saddr, sport, daddr, dport];
printf "%g %d\n", time, ownd > owndfile[saddr, sport, daddr, dport];
printf "%g %d\n", time, owndcorr > owndcorrfile[saddr, sport, daddr, dport];
printf "%g %g\n", time, srtt > srttfile[saddr, sport, daddr, dport];
printf "%g %g\n", time, rttvar > rttvarfile[saddr, sport, daddr, dport];
1} 
END {
	for (f in cwndfile) {
		close(cwndfile[f]);
	}
	for (f in ssthreshfile) {
		close(ssthreshfile[f]);
	}
	for (f in owndfile) {
		close(owndfile[f]);
	}
	for (f in owndcorrfile) {
		close(owndcorrfile[f]);
	}
	for (f in srttfile) {
		close(srttfile[f]);
	}
	for (f in rttvarfile) {
		close(rttvarfile[f]);
	}
 	for (i in starttime) {
		if (endtime[i] == starttime[i]) 
			print starttime[i], endtime[i], highest_ack[i];
		bw = calc_bw(0,highest_ack[i],starttime[i],endtime[i]);
# 		bw = (highest_ack[i]/(endtime[i] - starttime[i]))*8.0;
#		duration = endtime[i] - starttime[i];
		duration = 15;
		if (i in cwndtime) {
			ss_bw = calc_bw(cwndhiack[i],highest_ack[i],cwndtime[i],endtime[i]);
#			ss_bw = ((highest_ack[i] - cwndhiack[i])/(endtime[i] - cwndtime[i]))*8.0;
			ss_starttime = cwndtime[i] - starttime[i];
		}
		else {
			ss_bw = -1;
			ss_starttime = -1;
		}
		if (i in middletime) {
			sh_bw = calc_bw(middlehiack[i],highest_ack[i],middletime[i],endtime[i]);
#			sh_bw = ((highest_ack[i] - middlehiack[i])/(endtime[i] - middletime[i]))*8.0;
		}
		else {
			sh_bw = -1;
		}
		on_bw = -1;
		if (i in turnontime) {
			if (turnontime[i] < turnofftime[i]) {
#				on_bw = calc_bw(turnonhiack[i],turnoffhiack[i],turnontime[i],turnofftime[i]);
				on_bw = calc_bw(turnonhiack[i],turnoffhiack[i],turnon,turnoff);
#				on_bw = ((turnoffhiack[i] - turnonhiack[i])/(turnofftime[i] - turnontime[i]))*8.0;
			}
		}
 		print ind[i], duration, bw, ss_starttime, ss_bw, sh_bw, on_bw > "thruput";
		print ind[i], numrexmit[i] > "numrexmit";
 	}
	close("thruput");
	close("numrexmit");
	for (i in ind) {
		print ind[i] > "index.out"
			}
	close("index.out");

}

function calc_bw(startseq, endseq, starttime, endtime) {
	if (endtime > starttime ) {
		return ((endseq-startseq)*8.0/(endtime-starttime));
	}
	else {
		return -1;
	}
}
