#!/usr/local/bin/gawk -f
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

	s = sprintf("%d%c%d%c%d%c%d", saddr, SUBSEP, sport, SUBSEP, daddr, SUBSEP, dport);
	if (time >= starttime && time <= endtime) {
		if (!(s in startack)) {
			startack[s] = hiack;
		}
		if (hiack > endack[s]) {
			endack[s] = hiack;
		}
	}
}

END {
	for (s in startack) {
		split(s,a,SUBSEP);
		saddr = a[1];
		sport = a[2];
		daddr = a[3];
		dport = a[4];
		if (sport != 0)
			printf "(%d,%d)->(%d,%d) %g\n", saddr, sport, daddr, dport, (endack[s]-startack[s])*8/(endtime-starttime);
	}
}
		
