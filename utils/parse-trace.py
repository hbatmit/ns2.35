#! /usr/bin/python
import sys
fh=open(sys.argv[1])
duration=float(sys.argv[2])
drop_stats=dict()
send_stats=dict()
recv_stats=dict()

for line in fh.readlines() :
  records=line.split()
  action=records[0]
  src_node=int(records[2])
  dst_node=int(records[3])
  src_addr=int(records[8].split('.')[0])
  dst_addr=int(records[9].split('.')[0])
  pkt_size_bits=int(records[5])*8
  protocol=(records[4]);

  if ( action == "+" ) :
    #enqueued at the first router
    if (src_addr, dst_addr, protocol ) not in send_stats :
      send_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
    else : 
      send_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;

  if ( action == "r" ) :
    #received at the second router 
    if (src_addr, dst_addr, protocol ) not in recv_stats :
      recv_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
    else : 
      recv_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;
 
  if ( action == "d" ) :
    # dropped packet
    if (src_addr, dst_addr, protocol ) not in drop_stats :
      drop_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
    else : 
      drop_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;

for pair in send_stats :
  print "Pair : ",pair
  print "sent :",send_stats[pair]/duration, " bits/second"
  if pair not in recv_stats :
    print "recv :",0," bits/second"
  else :
    print "recv :",recv_stats[pair]/duration," bits/second"
  if pair not in drop_stats :
    print "dropped",0
  else :
    print "dropped",drop_stats[pair]/duration," bits/second"
  print "====================\n"
