#!/usr/bin/perl -w

$totalSize = 0;
$cur_time= 0;
$start=0;
$time_block = 0.001;
$cur_epoch = 0;

while (<STDIN>) {
      
        ($cur_time,$size) = split(' ',$_);

	if ($cur_time > $cur_epoch) {
	    while ($cur_epoch < $cur_time) {
	        print "$totalSize\n";
		$cur_epoch = $cur_epoch + $time_block;
	        $totalSize=0;
	    }
	    if ($cur_time < $cur_epoch) {$totalSize=$size;}
	} else {
	    $totalSize=$totalSize + $size;
	}
}
	
