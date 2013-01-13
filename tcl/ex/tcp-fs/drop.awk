#!/usr/local/bin/gawk -f
BEGIN {
	fsDrops = 0;
	otherDrops = 0;
}
($2 >= startTime && $2 <= endTime) {
	op = $1;
	time = $2;
	src = $9;
	dst = $10;
	if (match(src,"^0\.") && $1 == "+") {
		numFs++;
	}
	if (numFs > 0 && op == "d") {
		if (match(src,"^0\."))
			fsDrops++;
		else
			otherDrops++;
	}
	if (match(src,"^0\.") && ($1 == "-" || $1 == "d"))
		numFs--;
}

END {
	print otherDrops, fsDrops;
}

