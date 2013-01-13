#! /bin/csh

echo "Start Time"
date
foreach sim ( allUDP allTCP mix multi pktsVsBytes response singleVsMulti tfrc varying web testFRp testFRp_tcp PIdent) 

	echo "\nStarting $sim"
	${sim}/${sim}.sh

	#cleanup
	rm -f one.* multi.* data* tmp* dropRate* flow*
end
echo "\nEnd Time"
date
