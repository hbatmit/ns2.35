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
		print Destination 'v -t 0.003 -e sim_annotation 0.003 2 Nodes turn red when there is a collision ', "\n";
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

$i =3;
$one = 0;
$two = 0;
$three = 0;

$last_time = 0.1500;
$line = <Source>;
while ($line) {
	if ($line =~ /^M/) {
		@fields = split ' ', $line;
		@fields[3] =~ m/(\d+\.\d+)/;
		@fields[3] = $1;
		@fields[4] =~ m/(\d+\.\d+)/;
		@fields[4] = $1;
		@fields[6] =~ m/(\d+\.\d+)/;
		@fields[6] = $1;
		$t = (@fields[6] - @fields[3])/@fields[8];
		if ($t < 0)
		{
			$t = -$t;
		}
		if(@fields[3] > @fields[6])
		{
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -x ', @fields[3], ' -y ', @fields[4], ' -U -',@fields[8], ' -V 0.00 -T ', $t,"\n";
		}
		else
		{
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2],' -x ',@fields[3],' -y ',@fields[4],' -U ',@fields[8],' -V 0.00 -T ',$t,"\n";
		}
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1],' ', $i,' NODE 2 MOVES ',"\n";
		$last_time = @fields[1]+0.00005;
		$i++;
		if ($num_mov == 0)
		{
			print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 is out of range of Node 1',"\n";
		}
		if ($num_mov == 1)
		{
			print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 is at the border range of Node 1',"\n";
		}
		if ($num_mov == 2)
		{
			print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 is in range of Node 1, close enough to',"\n";
			$last_time = $last_time + 0.00005;
			$i++;
		print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' interfere with Node 0 but not the same distance as Node 0 from Node 1',"\n";
		}
		$i++;

		$num_mov ++;
		$last_time = $last_time + 0.08;
		$line = <Source>;
	}
	elsif ($num_mov < 1) { # Node 0 and Node 2 are equidistant from Node 1
		if ($line =~ /SENSING_CARRIER/) {
			@fields = split ' ', $line;
			$next_line = <Source>;
			if ($next_line =~ /BACKING_OFF/) {
				$other_node = 2;
				if (@fields[4] == 2)
				{
					$other_node = 0;
				}
				$t = $last_time +0.005;
				print Destination 'v -t ', $t, ' -e sim_annotation ', $t,' ', $i,' CASE 1a : EQUIDISTANT : SEPARATE TIMES : SUCCESSFUL RECEPTION',"\n";
				$last_time = $t+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 and Node 2 are in range of each other,',"\n"; 
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time, ' ', $i,' and are equidistant from Node 1, so when they both send packets at different times, the signal of these', "\n";
				$last_time = $last_time + 0.00005;
				$i++;
		print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,'packets, at the receiver ,is the same, and hence packets from both senders are received',"\n";
				$i++;
				$next_duration = $last_time + 0.01;

				print Destination '+ -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'r -t ', $next_duration, ' -s ', @fields[4], ' -d 1 -p message -e 2500 -a 1 ', "\n";

				$next_duration = $next_duration + 0.01;
				print Destination '+ -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'r -t ', $next_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 1 ', "\n";

				$last_time = $next_duration;
				$line = <Source>;
			}
			elsif ($next_line =~ /SENSING_CARRIER/) {
				@new_fields = split ' ', $next_line;
				$last_time = $last_time + 0.01;
				print Destination 'v -t ', $last_time, ' -e sim_annotation ', $last_time,' ', $i,' CASE 1b : EQUIDISTANT - SAME TIME : COLLISION ',"\n";
				$last_time = $last_time+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 and Node 2 want to send packets at the same time, since they are',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' the same distance from the receiver, the packets received are the same strength',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' and hence they interfere with each other resulting in collision',"\n";
				$i++;

				$duration = $last_time + 0.01;
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
				$last_time = $red_end_duration;
				$line = <Source>;
			}
			else {
				$line = <Source>;
			}

			}
			else {
			$line = <Source>;
			}
		}
		elsif (($num_mov == 1) && ($one == 0)) { # Node 2 is out of range of Node 1
				$duration = $last_time + 0.01;
				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 2a : NODE 2 OUT OF RANGE , NODES SENDING PACKETS SEPARATELY ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1, which are successfully received by Node 1',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;
				$last_time = $duration + 0.00005;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 sends data packets to Node 1',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ', $last_time,' -e sim_annotation ',$last_time,' ',$i,'	but they are not recd. by Node 1, since Node 2 is out of range of Node 1  ',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				$duration = $duration + 0.01;

				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 2b : NODE 2 OUT OF RANGE , NODES SENDING PACKETS AT THE SAME TIME ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1 which',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' are successfully received by Node 1  and Node 2 sends packets to Node 1',"\n";
			$last_time = $last_time + 0.00005;
			$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' which are not received, because Node 2 is out of range of Node 1',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				$duration = $duration + 0.01;

				$line = <Source>;
				$last_time = $duration;
				$one = 1;
		}
		elsif (($num_mov == 2) && ($two == 0)) # Node 2 is barely within range of Node 1
		{
				$duration = $last_time + 0.01;
				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 3a : NODE 2 BARELY IN RANGE OF NODE 1 , NODES SENDING PACKETS SEPARATELY ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1, which are successfully received by Node 1',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;
				$last_time = $duration + 0.00005;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 sends data packets to Node 1 which are received successfully by Node 1 ',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;

				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 3b : CHANNEL CAPTURE : NODE 2 BARELY IN RANGE OF NODE 1, NODES SENDING PACKETS AT THE SAME TIME ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1, which',"\n";					$last_time = $last_time + 0.00005;
				$i++;	
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' are successfully received by Node 1  and Node 2 sends packets to Node 1 which are not received, because',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' Node 0 being closer to Node 1, the signal strength of its packets is higher than that of Node 2 packets',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d -1 -p message -e 2500 -a 8 ', "\n";
				$last_time = $duration;
				$line = <Source>;

				$two = 1;
		}
		elsif(($num_mov == 3) && ($three == 0))# Node 2 is close enough to interfere with Node 0
		{
				$duration = $last_time + 0.01;
				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 4a : NODE 2 IN RANGE OF NODE 1 , NODES SENDING PACKETS SEPARATELY ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1, which are successfully received by Node 1',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;
				$last_time = $duration + 0.00005;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 2 sends data packets to Node 1 and they are received successfully by Node 1',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;

				print Destination 'v -t ', $duration, ' -e sim_annotation ', $duration,' ', $i,' CASE 4b : NODE 2 IN RANGE OF NODE 1 , NODES SENDING PACKETS AT THE SAME TIME ',"\n";
				$last_time = $duration+0.00005;
				$i++;
				print Destination 'v -t ', $last_time,' -e sim_annotation ', $last_time,' ',$i,' Node 0 sends data packets to Node 1 the same',"\n";
				$last_time = $last_time + 0.00005;
				$i++;
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,' time that Node 2 sends packets to Node 1, and hence they collide at the reciever since',"\n";
				$last_time = $last_time + 0.00005;
				$i++; 
			print Destination 'v -t ',$last_time,' -e sim_annotation ',$last_time,' ',$i,'their packets interfere with each other',"\n";
				$i++;
				$duration = $last_time + 0.00005;
				print Destination '+ -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 0 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '+ -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination '- -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				print Destination 'h -t ', $duration, ' -s 2 -d 1 -p message -e 2500 -a 1 ', "\n";
				$duration = $duration + 0.01;
				$red_duration = $duration + 0.01;
				$red_end_duration = $red_duration + 0.01;
				print Destination 'n -t ', $red_duration, ' -s 1 -S COLOR -c red -o black -i red -I black ', "\n";
				print Destination 'n -t ', $red_duration, ' -s 1 -S DLABEL -l "Collision " -L ""', "\n";
				print Destination 'd -t ', $red_duration, ' -s 1 -d 2 -p message -e 5000 -a 8 ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S COLOR -c black -o red -i black -I red ', "\n";
				print Destination 'n -t ', $red_end_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
				$last_time = $red_end_duration;

				$line = <Source>;

				$three = 1;
		}
		else {
			$line = <Source>;
		}
}
