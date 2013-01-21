#! /usr/bin/python
import sys
fh=open(sys.argv[1])
duration=float(sys.argv[2])
drop_stats=dict()
send_stats=dict()
recv_stats=dict()

# Per packet timestamps
enqueued=dict()
received=dict()

# Per flow delay lists
delay_stats=dict()

# Per flow percentiles and throughputs, for Jain Fairness
flow_percentiles=dict()
flow_throughputs=dict()

def jain_fainess( values ) :
  square_sum = ( sum( values ) )**2;
  sum_square = sum( map( lambda x : x**2, values ) )
  return square_sum/( len(values) * sum_square )

for line in fh.readlines() :
  records=line.split()
  action=records[0]
  src_node=int(records[2])
  dst_node=int(records[3])
  src_addr=int(records[8].split('.')[0])
  dst_addr=int(records[9].split('.')[0])
  pkt_size_bits=int(records[5])*8
  protocol=(records[4]);

  # delay calculations
  time_stamp=float(records[1])
  pkt_num = int(records[11]);

  if ( action == "+" ) :
    #enqueued at the first router
    if (src_addr, dst_addr, protocol ) not in send_stats :
      send_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
      delay_stats[ (src_addr, dst_addr, protocol) ] = [];
    else : 
      send_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;
    enqueued[ pkt_num ] = time_stamp;

  if ( action == "r" ) :
    #received at the second router 
    if (src_addr, dst_addr, protocol ) not in recv_stats :
      recv_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
    else : 
      recv_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;
    received[ pkt_num ] = time_stamp
    delay_stats[ (src_addr, dst_addr, protocol) ] += [received[pkt_num]- enqueued[pkt_num]]

  if ( action == "d" ) :
    # dropped packet
    if (src_addr, dst_addr, protocol ) not in drop_stats :
      drop_stats[ (src_addr, dst_addr, protocol) ] = pkt_size_bits;
    else : 
      drop_stats[ (src_addr, dst_addr, protocol) ] += pkt_size_bits;

for pair in send_stats :
  print "Pair :",pair
  print "sent :",send_stats[pair]/duration, "bits/second"
  if pair not in recv_stats :
    print "recv :",0,"bits/second"
  else :
    print "recv :",recv_stats[pair]/duration,"bits/second"
  if pair not in drop_stats :
    print "dropped",0
  else :
    print "dropped",drop_stats[pair]/duration,"bits/second"
  delay_stats[pair].sort();
  flow_throughputs[ pair ] = recv_stats[ pair ]/duration
  flow_percentiles[ pair ] = 1000*delay_stats[pair][int(float(sys.argv[3])*len(delay_stats[pair]))]
  print sys.argv[3]," %ile delay :", flow_percentiles[ pair ],"ms"
  print "====================\n"

print>>sys.stderr,"Aggregate statistics"
print>>sys.stderr,"Total throughput %.0f"%sum(flow_throughputs.values()), "bits/second"
print>>sys.stderr,"Average",sys.argv[3],"percentile %.0f"%(sum( flow_percentiles.values() )/len( flow_percentiles.values() )), "ms"
print>>sys.stderr,"Throughput fairness",jain_fainess( flow_throughputs.values() )
print>>sys.stderr,"Delay fairness", jain_fainess( map ( lambda x : 1.0/x , flow_percentiles.values() ) )
