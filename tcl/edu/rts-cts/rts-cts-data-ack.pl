# Script parses the trace file and using the base nam
# file, it generates a nam file that sets colors to
# nodes and data packets depending on their state.
# When a node is doing carrier sense, it turns red.
# When a node is sending RTS packets, it turns purple.
# RTS packets are purple broadcas packets.
# When a node is sending CTS packets, it turns green.
# CTS packets are green broadcast packets.
# When a node is sending data packets, it turns tan.
# Data packets are tan in color.
# When a node is sending ACK packets, it turns orange.
# ACK packets are orange in color.
sub usage {
	print STDERR "usage: $0 <Base trace file> <Base nam trace file> <output nam trace file>n";
	exit;
}

#@ARGV[0] - source trace file
#@ARGV[1] - source nam trace file
#@ARGV[2] - output nam trace file

open (Nam, $ARGV[1]) or die "Cannot open $ARGV[1] : $!\n";
open (Source, $ARGV[0]) or die "Cannot open $ARGV[0] : $!\n";
open (Destination, ">$ARGV[2]") or die "Cannot open $ARGV[2]: $!\n";


$line = <Nam>;
$tp =0;
while (($line) && ($tp ==0)) { 
	if ($line =~ /-i 3 -n green/) {
		print Destination $line;
		print Destination 'v -t 0.0 -e set_rate_ext 0.550ms 1', "\n";
		print Destination 'v -t 0.0 -e sim_annotation 0.0 1 In a Wireless network topology,when not all nodes are within each other\'s range, ',"\n";
		print Destination 'v -t 0.01 -e sim_annotation 0.001 2 carrier sense only provides information about collisions at the sender,not at ', "\n";
		print Destination 'v -t 0.015 -e sim_annotation 0.002 3 the receiver. This leads to problems like hidden terminal where two sender ', "\n";
		print Destination 'v -t 0.02 -e sim_annotation 0.003 4 nodes out of range of each other transmit packets at the same time, ', "\n";
		print Destination 'v -t 0.025 -e sim_annotation 0.004 5 to the same receiver, resulting in  collisions at the receiver. To solve ', "\n";
 		print Destination 'v -t 0.03 -e sim_annotation 0.005 6 such problems, control packets, RTS & CTS are used. When a node wants to ', "\n";
		print Destination 'v -t 0.035 -e sim_annotation 0.006 7 transmit packets, it first sends a RTS packet to the receiver.All nodes within ', "\n";
		print Destination 'v -t 0.04 -e sim_annotation 0.007 8 the sender\'s range receive this RTS packet. Every node hearing this RTS packet ', "\n";
		print Destination 'v -t 0.045 -e sim_annotation 0.008 9 will defer transmission. If receiver is not receiving data from any other node, ', "\n";
		print Destination 'v -t 0.05 -e sim_annotation 0.009 10 it responds with a CTS packet.This packet is again received by every node ', "\n";
		print Destination 'v -t 0.055 -e sim_annotation 0.01 11 within range of receiver. So all nodes in this range defer transmission. ', "\n";
		print Destination 'v -t 0.06 -e sim_annotation 0.02 12 Sender node sends data packet after receiving CTS. If sender node does not ', "\n";
		print Destination 'v -t 0.065 -e sim_annotation 0.03 13 hear the CTS, it will time-out and schedule retransmission of RTS. On ', "\n";
		print Destination 'v -t 0.07 -e sim_annotation 0.04 14 receiving data packet, receiver sends an ACK to sender.  ', "\n";
		$tp = 1;
	}
	else
	{
		print Destination $line;
		$line = <Nam>;
	}
}

print Destination 'v -t 0.100 -e sim_annotation 0.100 18 COLOR LEGEND : ', "\n";
print Destination 'v -t 0.110 -e sim_annotation 0.110 19 RTS packets : Purple color. Node turns purple  while sending an RTS packet ', "\n";
print Destination 'v -t 0.120 -e sim_annotation 0.120 20 CTS packets : Green color . Node turns  green while sending CTS packets ', "\n";
print Destination 'v -t 0.130 -e sim_annotation 0.130 21 Data packets : Tan color . Node turns tan while sending data packets ', "\n";
print Destination 'v -t 0.140 -e sim_annotation 0.140 22 ACK packets : Orange color. Node turns  orange while sending ACK packets ', "\n";
$line = <Source>;
@fields = split ' ', $line;
my $temp = @fields[1];
my $i = 23;
my $t = 1;
my $dw = 0;
my $num_mov = 0;
my $come = 0;
my $once = 0;

while($line) {
	@fields = split ' ', $line;
	@fields[1] = $temp + 0.005;
	# This generates movement of node in name file.
	if ($line =~ /^M/)
	{
		@fields[3] =~ m/(\d+\.\d+)/;
		@fields[3] = $1;
		@fields[4] =~ m/(\d+\.\d+)/;
		@fields[4] = $1;
		@fields[6] =~ m/(\d+\.\d+)/;
		@fields[6] = $1;
		$t = (@fields[6] - @fields[3])/@fields[8];
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -x ', @fields[3], ' -y ', @fields[4], ' -U ',@fields[8], ' -V 0.00 -T ', $t,"\n";
		$num_mov ++;
		if($num_mov == 1)
			{
			print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' NODE MOVES : Sender nodes can hear both RTS and CTS ', "\n";
		$i++;
			}
		elsif ($num_mov == 2)
			{
			print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' NODE MOVES : Sender nodes can hear CTS but not RTS', "\n";
		$i++;
			}
		$line = <Source>;
		$temp = @fields[1];
	}
	elsif( # This parses the trace file for RTS packets before node moves.
		($line =~ /s/) && ($line =~ /MAC/) && ($line =~ /RTS/) && (@fields[2] !~ /1/) && ($num_mov <= 1)
	   ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$other_node = 0;
		if (@fields[2] == 0)
		{
			$other_node = 2;
		}
		if ($come == 0)
		{
		$come = 1;
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' CASE 1 : NO CONTENTION. ', "\n";
		$i++;
		@fields[1] = @fields[1] + 0.00005;
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' Only one node sends data, it does a simple RTS-CTS_DATA-ACK exchange ', "\n";
		$i++;
		@fields[1] = @fields[1] + 0.00005; 
		print Destination 'n -t ', @fields[1], ' -s 0 -S COLOR -c red -o black -i red -I black', "\n";
		print Destination 'n -t ', @fields[1], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
		$duration = @fields[1] + 0.005;
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S COLOR -c purple -o red -i purple -I red', "\n";
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";
		print Destination '+ -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $duration, ' -s ', @fields[2], ' -d -1 p message -e 500 -a 7', "\n";
		$new_duration = $duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S DLABEL -l "" -L ""', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s 1 -S COLOR -c green -o black -i green -I black', "\n";
		print Destination 'n -t ', $new_duration, ' -s 1 -S DLABEL -l "Sending CTS packets" -L ""', "\n";
		print Destination '+ -t ', $new_duration, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination '- -t ', $new_duration, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'h -t ', $new_duration, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'r -t ', $new_duration, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s 1 -S COLOR -c black -o green -i black -I green', "\n";
		print Destination 'n -t ', $new_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S COLOR -c tan -o black -i tan -I black', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S DLABEL -l "Sending Data Packet" -L ""', "\n";
		print Destination '+ -t ', $new_duration, ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination '- -t ', $new_duration, ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'h -t ', $new_duration, ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'r -t ', $new_duration, ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S COLOR -c black -o tan -i black -I tan', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S DLABEL -l "" -L ""', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s 1 -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination 'n -t ', $new_duration, ' -s 1 -S DLABEL -l "Sending ACK packet" -L ""', "\n";
		print Destination '+ -t ', $new_duration, ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', $new_duration, ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', $new_duration, ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', $new_duration, ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		$new_duration = $new_duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s 1 -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $new_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		print Destination 'v -t ', $new_duration, ' -e sim_annotation ', $new_duration, ' ', $i, ' CASE 2 : BACKOFF DUE TO RTS', "\n"; 
		$i++;
		$new_duration = $new_duration + 0.005;
		print Destination 'v -t ', $new_duration , ' -e sim_annotation ', $new_duration, ' ' , $i, ' Nodes are in RTS range of each other but not in CTS range of the receiver ', "\n";
		$i++;
		$new_duration = $new_duration + 0.005;
		print Destination 'v -t ', $new_duration , ' -e sim_annotation ', $new_duration, ' ', $i, ' Node 0 want to send data to Node 1 at the same time that Node 2 wants to send data to Node 0 ', "\n";
		$i++;
		$new_duration = $new_duration + 0.005;
		print Destination 'v -t ', $new_duration, ' -e sim_annotation ', $new_duration, ' ', $i, ' thus Node 2 backsoff when it hears the RTS from Node 0', "\n";
		$i ++;
		$temp = $new_duration;
		@fields[1] = $new_duration + 0.01;
		}
		if($num_mov == 1)
		{
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' CASE 3 : BACKOFF AGAIN DUE TO RTS', "\n";
		$i++;
		$new_duration = @fields[1] + 0.00005;
		print Destination 'v -t ', $new_duration, ' -e sim_annotation ', $new_duration, ' ', $i, ' All nodes are in the RTS and CTS range of each other', "\n";
		$i++;
		$new_duration = $new_duration + 0.00005;
		print Destination 'v -t ', $new_duration, ' -e sim_annotation  ', $new_duration, ' ' , $i, ' Nodes 0 and 2 are in range of each other and hence can hear the RTS packet of the other node after hearing RTS packet node backs off', "\n";
		$i ++;
		@fields[1] = $new_duration + 0.01;
		}
		print Destination 'n -t ', @fields[1], ' -s 0 -S COLOR -c red -o black -i red -I black', "\n";
		print Destination 'n -t ', @fields[1], ' -s 2 -S COLOR -c red -o black -i red -I black', "\n";
		print Destination 'n -t ', @fields[1], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
		print Destination 'n -t ', @fields[1], ' -s 2 -S DLABEL -l "Carrier sense" -L ""', "\n";
		$duration = @fields[1] + 0.005;
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S COLOR -c purple -o red -i purple -I red', "\n";
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";
		print Destination '+ -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";

		$new_duration = $duration + 0.01;
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S DLABEL -l "" -L ""', "\n";

		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S COLOR -c DarkOliveGreen -o red -i DarkOliveGreen -I red', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S DLABEL -l "Data Waiting" -L ""', "\n";
		$i ++;
		$line = <Source>;
		$temp = $new_duration;
		}
	elsif( # This parses trace file for RTS packets after node moves
		($line =~ /s/) && ($line =~ /MAC/) && ($line =~ /RTS/) && (@fields[2] !~ /1/) && ($num_mov > 1)
	  ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$other_node = 0;
		if (@fields[2] == 0)
		{
			$other_node = 2;
		}
		if ($once == 0) {
		print Destination 'v -t ', @fields[1], ' -e sim_annotation  ', @fields[1], ' ' , $i, ' CASE 4 : BACKOFF DUE TO CTS ', "\n";
		$i++;
		@fields[1] = @fields[1] + 0.005;
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' Senders are out of range of RTS of each other but are in  range of CTS of receiver', "\n";
		$i ++;
		@fields[1] = @fields[1] + 0.005;
		print Destination 'v -t ', @fields[1], ' -e sim_annotation ', @fields[1], ' ', $i, ' Node 0 sends RTS which Node 2 does not hear but it hears the CTS sent by receiver and thus backsoff after hearing CTS, thus avoiding collision at the receiver', "\n";
		$i ++;
		@fields[1] = @fields[1] + 0.005;

		$once ++;
		}
		if ($dw == 0) {
			print Destination 'n -t ', @fields[1], ' -s 0 -S COLOR -c red -o black -i red -I black', "\n";
			print Destination 'n -t ', @fields[1], ' -s 2 -S COLOR -c red -o black -i red -I black', "\n";
			print Destination 'n -t ', @fields[1], ' -s 0 -S DLABEL -l "Carrier sense" -L ""', "\n";
			print Destination 'n -t ', @fields[1], ' -s 2 -S DLABEL -l "Carrier sense" -L ""', "\n";
		}
		if($dw == 1) {
			print Destination 'n -t ', @fields[1],' -s ', @fields[2],' -S COLOR -c red -o black -i red -I black', "\n";
			print Destination 'n -t ', @fields[1],' -s ', @fields[2],' -S DLABEL -l "Carrier sense" -L ""', "\n";
		}

		$duration = @fields[1] + 0.005;
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S COLOR -c purple -o red -i purple -I red', "\n";
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";
		print Destination '+ -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $duration, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 7', "\n";
		$new_duration = $duration + 0.005;
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', @fields[2], ' -S DLABEL -l "" -L ""', "\n";
		$temp = $new_duration;
		if ($t == 1) {
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S COLOR -c purple -o red -i purple -I red', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";
		print Destination '+ -t ', $new_duration, ' -s ', $other_node, ' -d  -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $new_duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $new_duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $new_duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		$final_duration = $new_duration + 0.005;
		print Destination 'n -t ', $final_duration, ' -s ', $other_node, ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $final_duration, ' -s ', $other_node, ' -S DLABEL -l "" -L ""', "\n";
		$t++;
		$temp = $final_duration;
		}
		$line = <Source>;
	    }
	elsif( # This parses trace file for CTS packets after node moves
		($line =~ /s/) && ($line =~ /MAC/) && ($line =~ /CTS/) && (@fields[2] !~ /0/) && (@fields[2] !~ /2/) && ($num_mov >= 1)
	     ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$jitter = @fields[1] + 0.01;
		print Destination 'n -t ', $jitter, ' -s 1 -S DLABEL -l "Sending CTS packets" -L ""', "\n";
		print Destination 'n -t ', $jitter, ' -s 1 -S COLOR -c green -o black -i green -I black', "\n";
		print Destination '+ -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination '- -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'h -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'r -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		$duration = $jitter + 0.01;
		print Destination 'n -t ', $duration, ' -s 1 -S COLOR -c black -o green -i black -I green', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;
		$line = <Source>;
		$temp = $duration;
		}
	elsif( # This parses trace file for CTS packets after node moves
		($line =~ /s/) && ($line =~ /MAC/) && ($line =~ /CTS/) && (@fields[2] !~ /0/) && (@fields[2] !~ /2/) && ($num_mov == 0)
	     ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$jitter = @fields[1] + 0.01;

		print Destination 'n -t ', $jitter, ' -s 1 -S DLABEL -l "Sending CTS packets" -L ""', "\n";
		print Destination 'n -t ', $jitter, ' -s 1 -S COLOR -c green -o black -i green -I black', "\n";
		print Destination '+ -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination '- -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'h -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'r -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		$duration = $jitter + 0.01;
		print Destination 'n -t ', $duration, ' -s 1 -S COLOR -c black -o green -i black -I green', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;
		$line = <Source>;
		$temp = $duration;
		}
	elsif( # This parses trace file for data packets
		($line =~ /s/) && ($line =~ /MAC/) && ($line =~ /message/) && (@fields[2] !~ /1/)
	     ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$other_node = 0;
		if(@fields[2] == 0) {
			$other_node = 2;
		}
		@fields[1] = @fields[1] +0.01;
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -S DLABEL -l "Sending Data packets" -L ""', "\n";
		print Destination 'n -t ', @fields[1], ' -s ', @fields[2], ' -S COLOR -c tan -o black -i tan -I black', "\n";
		if(($num_mov > 1) && ($dw == 0)) {
			print Destination 'n -t ', @fields[1], ' -s ', $other_node, ' -S COLOR -c DarkOliveGreen -o black -i DarkOliveGreen -I black', "\n";
			print Destination 'n -t ', @fields[1], ' -s ', $other_node, ' -S DLABEL -l "Data waiting" -L ""', "\n";
		}
		print Destination '+ -t ', @fields[1], ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination '- -t ', @fields[1], ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'h -t ', @fields[1], ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'r -t ', @fields[1], ' -s ', @fields[2], ' -d 1 -p message -e 2500 -a 6', "\n";
		$duration = @fields[1] + 0.01;
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S COLOR -c black -o tan -i black -I tan', "\n";
		print Destination 'n -t ', $duration, ' -s ', @fields[2], ' -S DLABEL -l "" -L "" ', "\n";
		$i++;
		$line = <Source>;
		$temp = $duration;
		}
	elsif( # This parses trace file for ACK packets
		($line =~ /r/) && ($line =~ /MAC/) && ($line =~ /ACK/) && (@fields[2] !~ /1/) && ($num_mov > 1)
	     ) {
		$dw ++;
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		print Destination 'n -t ', @fields[1], ' -s 1 -S DLABEL -l "Sending ACK packets" -L ""', "\n";
		print Destination 'n -t ', @fields[1], ' -s 1 -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination '+ -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		$duration = @fields[1] + 0.01;
		print Destination 'n -t ', $duration, ' -s 1  -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;
		$line = <Source>;
		$temp = $duration;
		}
	elsif( # This parses trace file for ACK packets.
		($line =~ /r/) && ($line =~ /MAC/) && ($line =~ /ACK/) && (@fields[2] !~ /1/) && ($num_mov == 0)
	     ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$other_node = 0;
		if(@fields[2] == 0) {
			$other_node = 2;
		}
		print Destination 'n -t ', @fields[1], ' -s 1 -S DLABEL -l "Sending ACK packet" -L ""', "\n";
		print Destination 'n -t ', @fields[1], ' -s 1 -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination '+ -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		$duration = @fields[1] + 0.01;
		print Destination 'n -t ', $duration, ' -s 1  -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;


		print Destination 'n -t ', $duration, ' -s ', $other_node, ' -S COLOR -c purple -o DarkOliveGreen -i purple -I DarkOliveGreen', "\n";
		print Destination 'n -t ', $duration, ' -s ', $other_node, ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";
		print Destination '+ -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		$new_duration = $duration + 0.005;
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S DLABEL -l "" -L ""', "\n";
		$i ++;

		$jitter = $new_duration + 0.01;
		print Destination 'n -t ', $jitter, ' -s ', @fields[2], ' -S DLABEL -l "Sending CTS packets" -L ""', "\n";
		print Destination 'n -t ', $jitter, ' -s ', @fields[2], ' -S COLOR -c green -o black -i green -I black', "\n";
		print Destination '+ -t ', $jitter, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 3', "\n";
		print Destination '- -t ', $jitter, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'h -t ', $jitter, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'r -t ', $jitter, ' -s ', @fields[2], ' -d -1 -p message -e 500 -a 3', "\n";
		$n_duration = $jitter + 0.01;
		print Destination 'n -t ', $n_duration, ' -s ', @fields[2], ' -S COLOR -c black -o green -i black -I green', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', @fields[2],' -S DLABEL -l "" -L ""', "\n";
		$i++;

		$n_duration = $n_duration + 0.005;
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S DLABEL -l "Sending Data packets" -L ""', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S COLOR -c tan -o DarkOliveGreen -i tan -I DarkOliveGreen', "\n";

		print Destination '+ -t ', $n_duration, ' -s ', $other_node, ' -d ', @fields[2], ' -p message -e 2500 -a 6', "\n";
		print Destination '- -t ', $n_duration, ' -s ', $other_node, ' -d ', @fields[2], ' -p message -e 2500 -a 6', "\n";
		print Destination 'h -t ', $n_duration, ' -s ', $other_node, ' -d ', @fields[2], ' -p message -e 2500 -a 6', "\n";
		print Destination 'r -t ', $n_duration, ' -s ', $other_node, ' -d ', @fields[2], ' -p message -e 2500 -a 6', "\n";
		$n_duration = $n_duration + 0.01;
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S COLOR -c black -o tan -i black -I tan', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S DLABEL -l "" -L "" ', "\n";
		$i++;

		$n_duration = $n_duration + 0.01;
		print Destination 'n -t ', $n_duration, ' -s ', @fields[2],' -S DLABEL -l "Sending ACK packet" -L ""', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', @fields[2],' -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination '+ -t ', $n_duration, ' -s ', @fields[2],' -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', $n_duration, ' -s ', @fields[2],' -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', $n_duration, ' -s ', @fields[2],' -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', $n_duration, ' -s ', @fields[2],' -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		$duration = $n_duration + 0.01;
		print Destination 'n -t ', $duration, ' -s ', @fields[2],'  -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $duration, ' -s ', @fields[2],' -S DLABEL -l "" -L ""', "\n";
		$i++;



		$line = <Source>;
		$temp = $duration;

		}
	elsif(
		($line =~ /r/) && ($line =~ /MAC/) && ($line =~ /ACK/) && (@fields[2] !~ /1/) && ($num_mov == 1)
	     ) {
		@fields[2] =~ m/(\d)/;
		@fields[2] = $1;
		$other_node = 0;
		if(@fields[2] == 0) {
			$other_node = 2;
		}
		print Destination 'n -t ', @fields[1], ' -s 1 -S DLABEL -l "Sending ACK packet" -L ""', "\n";
		print Destination 'n -t ', @fields[1], ' -s 1 -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination '+ -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', @fields[1], ' -s 1 -d ', @fields[2], ' -p message -e 500 -a 2', "\n";
		$duration = @fields[1] + 0.01;
		print Destination 'n -t ', $duration, ' -s 1  -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;

		print Destination 'n -t ', $duration, ' -s ', $other_node, ' -S COLOR -c purple -o DarkOliveGreen -i purple -I DarkOliveGreen', "\n";
		print Destination 'n -t ', $duration, ' -s ', $other_node, ' -S DLABEL -l "Sending RTS packet" -L ""', "\n";

		print Destination '+ -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination '- -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'h -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";
		print Destination 'r -t ', $duration, ' -s ', $other_node, ' -d -1 -p message -e 500 -a 7', "\n";

		$new_duration = $duration + 0.005;
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S COLOR -c black -o purple -i black -I purple', "\n";
		print Destination 'n -t ', $new_duration, ' -s ', $other_node, ' -S DLABEL -l "" -L ""', "\n";
		$i ++;

		$jitter = $new_duration + 0.01;
		print Destination 'n -t ', $jitter, ' -s 1 -S DLABEL -l "Sending CTS packets" -L ""', "\n";
		print Destination 'n -t ', $jitter, ' -s 1 -S COLOR -c green -o black -i green -I black', "\n";
		print Destination '+ -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination '- -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'h -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		print Destination 'r -t ', $jitter, ' -s 1 -d -1 -p message -e 500 -a 3', "\n";
		$n_duration = $jitter + 0.01;
		print Destination 'n -t ', $n_duration, ' -s 1 -S COLOR -c black -o green -i black -I green', "\n";
		print Destination 'n -t ', $n_duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";

		$n_duration = $n_duration + 0.005;
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S DLABEL -l "Sending Data packets" -L ""', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S COLOR -c tan -o DarkOliveGreen -i tan -I DarkOliveGreen', "\n";

		print Destination '+ -t ', $n_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination '- -t ', $n_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'h -t ', $n_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 6', "\n";
		print Destination 'r -t ', $n_duration, ' -s ', $other_node, ' -d 1 -p message -e 2500 -a 6', "\n";
		$n_duration = $n_duration + 0.01;
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S COLOR -c black -o tan -i black -I tan', "\n";
		print Destination 'n -t ', $n_duration, ' -s ', $other_node, ' -S DLABEL -l "" -L "" ', "\n";
		$i++;

		$n_duration = $n_duration + 0.01;
		print Destination 'n -t ', $n_duration, ' -s 1 -S DLABEL -l "Sending ACK packet" -L ""', "\n";
		print Destination 'n -t ', $n_duration, ' -s 1 -S COLOR -c orange -o black -i orange -I black', "\n";
		print Destination '+ -t ', $n_duration, ' -s 1 -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination '- -t ', $n_duration, ' -s 1 -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination 'h -t ', $n_duration, ' -s 1 -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		print Destination 'r -t ', $n_duration, ' -s 1 -d ', $other_node, ' -p message -e 500 -a 2', "\n";
		$duration = $n_duration + 0.01;
		print Destination 'n -t ', $duration, ' -s 1  -S COLOR -c black -o orange -i black -I orange', "\n";
		print Destination 'n -t ', $duration, ' -s 1 -S DLABEL -l "" -L ""', "\n";
		$i++;

		$line = <Source>;
		$temp = $duration;

		}

	else {
		$line = <Source>;
		$temp = @fields[1];
	     }
}

close Source;
close Destination;
close Nam;
