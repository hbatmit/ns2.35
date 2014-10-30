{
if ($4 != 0) {
  	if (($1 == src) && ($2 == dst) && ($4 == size)) {
		t=$3-time
		if ((t < 0.001) && ($4 > 500)) {
			if (($1 != preS) || ($2 != preD)) {
#				print $1, $2, $4
				print $4
				preS=$1
				preD=$2
			}
		}
	}
}
src=$1
dst=$2
time=$3
size=$4
}
