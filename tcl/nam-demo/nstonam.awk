#!/usr/bin/awk

#
# nam fmt:
#	eventCode time src dst size attr type conv id
#
# ns fmt:
#	eventCode time src dst pktType size flags class src.sport dst.dport
#		seqno uid
#

$2 ~ /testName/ {
  next;
}
$1 ~ /v/ {
  print $0;
  next;
}
$1 ~ /[dh+-]/ {
	print $1, $2, $3, $4, $6, $8, $5, $8, $12
	if ($1 == "h") {
		group = int($10) % 256;
		if ($3 < $4)
			key = $3 $4;
		else
			key = $4 $3;

		if (subscription[key] < group) {
			subscription[key] = group;
			print "v " $2 " $theNet ecolor " $3 " " $4 " $colorMap(" group ")"
			print "v " $2 " $theNet ecolor " $4 " " $3 " $colorMap(" group ")"
		}
	}
	if ($5 == "prune") {
		group = int($10) % 256;
		if ($3 < $4)
			key = $3 $4;
		else
			key = $4 $3;

		if (subscription[key] == group) {
			group -= 1;
			subscription[key] = group;
			print "v " $2 " $theNet ecolor " $3 " " $4 " $colorMap(" group ")"
			print "v " $2 " $theNet ecolor " $4 " " $3 " $colorMap(" group ")"
		}
	}
	#
	# print a hop event for each deque
	# (ns-2 doesn't dump hop events by default since
	# we can infer them from the "-" event)
	#
	if ($1 == "-")
		print "h", $2, $3, $4, $6, $8, $5, $8, $12
		
}

