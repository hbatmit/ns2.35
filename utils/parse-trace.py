#! /usr/bin/python
import sys
fh=open(sys.argv[1])
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

  if ( dst_node == 0 ) and ( action == "+" ) :
    #sending into first router
    if (src_addr,dst_addr ) not in send_stats :
      send_stats[ (src_addr,dst_addr) ] = 1;
    else : 
      send_stats[ (src_addr,dst_addr) ] +=1;

  if ( src_node == 1 ) and ( action == "r" ) :
    #sending out of the second router 
    if (src_addr,dst_addr ) not in recv_stats :
      recv_stats[ (src_addr,dst_addr) ] = 1;
    else : 
      recv_stats[ (src_addr,dst_addr) ] += 1;
 
  if ( src_node == 0 ) and ( dst_node == 1 ) and ( action == "d" ) :
    # dropped packet
    if (src_addr,dst_addr ) not in drop_stats :
      drop_stats[ (src_addr,dst_addr) ] = 1;
    else : 
      drop_stats[ (src_addr,dst_addr) ] +=1;

for pair in send_stats :
  print "Pair : ",pair
  print "sent :",8000*send_stats[pair], " bits"
  if pair not in recv_stats :
    print "recv",0
  else :
    print "recv :",8000*recv_stats[pair]," bits"
  if pair not in drop_stats :
    print "dropped",0
  else :
    print "dropped",8000*drop_stats[pair]," bits"
  print "====================\n"
