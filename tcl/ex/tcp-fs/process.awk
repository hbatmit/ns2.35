#!/usr/local/bin/gawk -f
BEGIN {
	erriter = -1;
}
{
	iter = $1;
	if (iter == erriter) 
		next;
	
	s = "";
	i = 2;
	while (index($i,"-")) {
		s = sprintf("%s %s", s, $i);
		i++;
	}
	s = sprintf("%s %s %s %s", s, $i, $(i+1), $(i+2));
	i += 3;
	error = 0;
	for (j = i; j <= NF; j++) {
		if ($j == -1)
			error = 1;
	}
	if (!error) {
		for (j = i; j <= NF; j++) {
			sum[s,j-i+1] += $j;
			sum2[s,j-i+1] += ($j*$j);
		}
		count[s]++;
	}
	else 
		erriter = iter;
	numval = NF - i + 1;
}
		

END {
	for (s in count) {
		printf "%-45s ", s;
		for (j = 1; j <= numval; j++) {
			mean = sum[s,j]/count[s];
			var = sum2[s,j]/count[s] - mean*mean;
			printf "%6.2f %6.2f ", mean, sqrt(var/count[s]);
		}
		printf "\n";
	}
}


		
