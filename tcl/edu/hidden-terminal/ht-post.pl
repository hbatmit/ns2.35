# Script parses trace file for hidden terminal.
# It sets colors to nodes and packets depending on their
# state.
# While doing carrier sense, nodes turn green. 
# When nodes backoff, they turn purple.
# When there is a collision at the reciever, it turns red.

sub usage {
	print STDERR "usage: $0 <Base Trace file> <Base nam trace file> <output nam trace file>n";
	exit;
}

#@ARGV[0] - source trace file
#@ARGV[1] - source nam trace file
#@ARGV[2] - output nam trace file

open (Source, $ARGV[0]) or die "Cannot open $ARGV[0] : $!\n";
open (NamSource, $ARGV[1]) or die "Cannot open $ARGV[1] : $!\n";
open (Destination, ">$ARGV[2]") or die "Cannot open $ARGV[2]: $!\n";

$namline = <NamSource>;
while ($namline) { 
	if ($namline =~ /-i 3 -n green/) {
		print Destination $namline;
		print Destination 'v -t 0.000 -e sim_annotation 0.0 1 COLOR LEGEND : ', "\n";
		print Destination 'v -t 0.001 -e sim_annotation 0.001 2 Nodes turn green when they are sensing carrier ', "\n";
		print Destination 'v -t 0.002 -e sim_annotation 0.002 3 Nodes turn purple when they backoff ', "\n";
		print Destination 'v -t 0.003 -e sim_annotation 0.003 4 Nodes turn red when there is a collision ', "\n";
		print Destination 'v -t 0.10000000 -e set_rate_ext 0.200ms 1', "\n";
		last;
	}
	else
	{
		print Destination $namline;
		$namline = <NamSource>;
	}
}

$num_mov = 0;

$i =5;

$last_time = 0.1000;
$line = <Source>;
while ($line) {
	if ($line =~ /SENSING_CARRIER/) {  # parses the trace file in case of a Carrier Sense event.
		@fields = split ' ', $line;
		$next_line = <Source>;
		if ($next_line =~ /BACKING_OFF/) { # parses trace file in case of a BACK_OFF event.
			$other_node = 2;
			if (@fields[4] == 2)
			{
				$other_node = 0;
			}
			
			$t = @fields[2] +0.005;

			print Destination 'n -t ',@fields[2], ' -s ', @fields[4], ' -S COLOR -c green -o black -I black ', "\n";
			print Destination 'n -t ',@fields[2], ' -s ',@fields[4],' -S DLABEL -l "Carrier sense" -L ""', "\n";
			print Destination 'v -t ',@fields[2], ' -e sim_annotation ', @fields[2],' ',$i,' CASE 1 : NO CONTENTION ',"\n";
			$last_time = @fields[2]+ 0.00005;
			$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ', $last_time,' ', $i,' Only Node 2 is sending data packets and therefore no contention',"\n";
			$last_time = $last_time+0.00005;
			$i++;
			$next_duration = @fields[2] + 0.01;
			
			print Destination 'n -t ', $next_duration, ' -s ', @fields[4], ' -S COLOR -c black -o green -i black -I green ', "\n";
			print Destination 'n -t ', $next_duration, ' -s ', @fields[4], ' -S DLABEL -l "" -L ""', "\n";
			print Destination '+ -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination '- -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'h -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'r -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";

			@fields[2] = $next_duration + 0.01;

			print Destination 'n -t ', @fields[2], ' -s ', @fields[4], ' -S COLOR -c green -o black -i green -I black ', "\n";
			print Destination 'n -t ', @fields[2], ' -s ', @fields[4], ' -S DLABEL -l "Carrier sense" -L ""', "\n";
			print Destination 'v -t ', @fields[2], ' -e sim_annotation ', @fields[2],' ', $i,' CASE 2 : BACKOFF ',"\n";
			$last_time = @fields[2]+0.00005;
			$i++;
			print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 and Node 2 are in range of each other, they do carrier sense at slightly different times',"\n";
			$last_time = $last_time + 0.00005;
			$i++;
			print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time,' ', $i,' so Node 0 finds the channel not free, and thus backs off', "\n";
			$last_time = $last_time+0.00005;
			$i++;
			$next_duration = @fields[2] + 0.01;

			print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S COLOR -c green -o black -i green -I black ', "\n";
			print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S DLABEL -l "Carrier Sense" -L ""', "\n";

			print Destination 'n -t ', $next_duration,' -s ', @fields[4], ' -S COLOR -c black -o green -i black -I green ', "\n";
			print Destination 'n -t ', $next_duration, ' -s ', @fields[4], ' -S DLABEL -l "" -L ""', "\n";
			print Destination '+ -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination '- -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'h -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'r -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";

			$next_duration = $next_duration + 0.005;
			print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S COLOR -c purple -o green -i purple -I green ', "\n";
			print Destination 'n -t ', $next_duration, ' -s ', $other_node, ' -S DLABEL -l "Backing off" -L ""', "\n";

			$next_duration = $next_duration + 0.005;
			print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S COLOR -c green -o purple -i green -I purple ', "\n";
			print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S DLABEL -l "Carrier sense" -L ""', "\n"; 			
			$next_duration = $next_duration + 0.005;
			print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S COLOR -c black -o green -i black -I green ', "\n";
			print Destination 'n -t ',$next_duration,' -s ', $other_node,' -S DLABEL -l "" -L ""', "\n";
			print Destination '+ -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination '- -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'h -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
			print Destination 'r -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";

			$line = <Source>;
		}
		elsif (($next_line =~ /SENSING_CARRIER/) && ($num_mov == 0)) {
				@new_fields = split ' ', $next_line;
				print Destination 'n -t ', @new_fields[2] , ' -s 0 -S COLOR -c green -o black -i green -I black ',"\n";
				print Destination 'n -t ', @new_fields[2], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
				print Destination 'n -t ', @new_fields[2] , ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', @new_fields[2], ' -s 2 -S DLABEL -l "Carrier Sense" -L ""' , "\n";

				print Destination 'v -t ', @new_fields[2], ' -e sim_annotation ', @new_fields[2],' ', $i,' CASE 3 : COLLISION WHEN NODES SEND AT SAME TIME  ',"\n";
				$last_time = @new_fields[2]+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Sender nodes are in range of each other but they do carrier sense at the same time,  ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time, ' ',$i,' thus finding channel to be free , so they send packets at the same time and therefore result in collision at the receiver',"\n";
				$i++;
				$last_time = $last_time +0.00005;

				$duration = @new_fields[2] + 0.01;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$red_duration = $duration + 0.01;
				$red_end_duration = $red_duration + 0.01;
				print Destination 'n -t ', $red_duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $red_duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $red_duration, ' -s 1 -d 2 -p message -e 5000 -a 8 ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$line = <Source>;
		}
		elsif(($next_line !~ /SENSING_CARRIER/) && ($num_mov > 0)) {

				print Destination 'n -t ', @fields[2] , ' -s 0 -S COLOR -c green -o black -i green -I black ',"\n";
				print Destination 'n -t ', @fields[2], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
				@fields[2] = @fields[2] + 0.005;
				print Destination 'n -t ', @fields[2] , ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', @fields[2], ' -s 2 -S DLABEL -l "Carrier Sense" -L ""' , "\n";

				print Destination 'v -t ', @fields[2], ' -e sim_annotation ', @fields[2],' ', $i,' CASE 4 : SUCCESSFUL TRANSMISSION WHEN NODES ARE OUT OF RANGE OF EACH OTHER',"\n";
				$last_time = @fields[2]+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Sender nodes are out of range of each other ', "\n";
				$last_time = $last_time + 0.00005;
				$i++;
				print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time,' ', $i, '  but they result in successful transmission since they send packets at different times',"\n";
				$last_time = $last_time+0.00005;
				$i++;
			
				$duration = @fields[2] + 0.005;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";

				$duration = $duration + 0.005;
				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$line = <Source>;
				$line = <Source>;
				$line = <Source>;
		}
		elsif(($next_line =~ /SENSING_CARRIER/) && ($num_mov > 0)) {
				print Destination 'n -t ', @fields[2] , ' -s 0 -S COLOR -c green -o black -i green -I black ',"\n";
				print Destination 'n -t ', @fields[2], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
				print Destination 'n -t ', @fields[2] , ' -s 2 -S COLOR -c green -o black -i green -I black ', "\n";
				print Destination 'n -t ', @fields[2], ' -s 2 -S DLABEL -l "Carrier Sense" -L ""' , "\n";


				print Destination 'v -t ', @fields[2], ' -e sim_annotation ', @fields[2],' ', $i,' CASE 5 : COLLISION IN A HIDDEN TERMINAL SCENARIO',"\n";
				$last_time = @fields[2]+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Sender nodes are out of range of each other ', "\n";
				$last_time = $last_time + 0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ', $i,' even though they both do carrier sense, they cannot hear each other and thus find the channel free ', "\n";
				$i++;
				$last_time = $last_time + 0.00005;
				print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time,' ', $i,' and they send packets at the same time, thus resulting in a collision at the receiver. ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				$duration = @fields[2] + 0.005;
				print Destination 'n -t ', $duration, ' -s 0 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 0 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";

				print Destination 'n -t ', $duration, ' -s 2 -S COLOR -c black -o green -i black -I green ', "\n";
				print Destination 'n -t ', $duration, ' -s 2 -S DLABEL -l "" -L ""', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$red_duration = $duration + 0.01;
				$red_end_duration = $red_duration + 0.01;
				print Destination 'n -t ', $red_duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $red_duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $red_duration, ' -s 1 -d 2 -p message -e 5000 -a 8 ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$line = <Source>;
				$line = <Source>;
				$line = <Source>;
		}

		else {
			$line = <Source>;
		} 
	}
	elsif ($line =~ /^M/) {  # parses trace file when node moves.
		@fields = split ' ', $line;
		@fields[3] =~ m/(\d+\.\d+)/;
		@fields[3] = $1;
		@fields[4] =~ m/(\d+\.\d+)/;
		@fields[4] = $1;
		@fields[6] =~ m/(\d+\.\d+)/;
		@fields[6] = $1;
		$t = (@fields[6] - @fields[3])/@fields[8];

		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1],' ', $i,' HIDDEN TERMINAL SCENARIO : Node 2 moves and hence is out of range of node 0',"\n";
		$last_time = @fields[1]+0.00005;
		$i++;

		@fields[1] = @fields[1] + 0.005;
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -x ', @fields[3], ' -y ', @fields[4], ' -U ',@fields[8], ' -V 0.00 -T ', $t,"\n";
		@fields[1] = @fields[1] + 0.100000;


		$num_mov ++;
		$line = <Source>;
	}
	else {
		$line = <Source>;
	}
}

close Source;
close NamSource;
close Destination;
