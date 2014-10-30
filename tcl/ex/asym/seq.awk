BEGIN {
	if (dir == "")
		dir = ".";
}	

{
if ($1 == "-" && ($5 == "tcp" || $5 == "ack")) {
	split($9,a,"\.");
	saddr = a[1];
	sport = a[2];
	split($10,a,"\.");
	daddr = a[1];
	dport = a[2];
	
	if ($5 == "tcp" && !((saddr, sport, daddr, dport) in indexarray)) {
		indexarray[saddr,sport,daddr,dport] = sprintf("%d,%d-%d,%d", saddr, sport, daddr, dport);
	}
	else if ($5 == "ack" && !((daddr, dport, saddr, sport) in  indexarray)) {
		indexarray[daddr,dport,saddr,sport] = sprintf("%d,%d-%d,%d", daddr, dport, saddr, sport);
	}
	
	if ($5 == "tcp" && !((saddr, sport, daddr, dport) in seqfile)) {
		seqfile[saddr,sport,daddr,dport] = sprintf("%s/seq-%d,%d-%d,%d.out", dir, saddr, sport, daddr, dport);
		printf "TitleText: (%d,%d)->(%d,%d)\n", saddr, sport, daddr, dport > seqfile[saddr,sport,daddr,dport];
		printf "Device: Postscript\n" > seqfile[saddr,sport,daddr,dport];
	}
	else if ($5 == "ack" && !((daddr, dport, saddr, sport) in ackfile)) {
		ackfile[daddr,dport,saddr,sport] = sprintf("%s/ack-%d,%d-%d,%d.out", dir, daddr, dport, saddr, sport);
		printf "TitleText: (%d,%d)->(%d,%d)\n", daddr, dport, saddr, sport > ackfile[daddr,dport,saddr,sport];
		printf "Device: Postscript\n" > ackfile[daddr,dport,saddr,sport];
	}
	
	lsrc = $3;
	ldst = $4;
	time = $2;
	seqno = $11;
# log both tcp and ack pkts at the source
	if ($5 == "tcp" && lsrc == saddr) {
		printf "%g %d\n", time, seqno > seqfile[saddr,sport,daddr,dport];
	}
	else if ($5 == "ack" && ldst == daddr) {
		printf "%g %d\n", time, seqno > ackfile[daddr,dport,saddr,sport];
	} 
}
}
END {
	for (f in seqfile) {
		close(seqfile[f]);
	}
	for (f in ackfile) {
		close(ackfile[f]);
	}
	for (i in indexarray) {
		print indexarray[i] > "seq-index.out"
			}
	close("seq-index.out");
}


