#!/usr/local/bin/gawk -f
BEGIN {
	s = "";
}

{
	if ($1 != "(0,0)->(3,0)") {
		sum += $6;
		s = sprintf("%s %d", s, $6);
	}
}

END {
	printf "%s %d", s, sum;
}
