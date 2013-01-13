{
t=$1 - s
if (s != 0) {
	p=1
	print t
}
s=$1
}
END {
	if (p != 1) {
		print 0
	}
}
