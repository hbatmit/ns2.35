#!/usr/local/bin/gawk -f
{
	if (NR > 1) 
		sum += ($3 - $2);
} 

END {
	if (NR > 1) 
		print sum/(NR-1); 
	else 
		print -1;
}
