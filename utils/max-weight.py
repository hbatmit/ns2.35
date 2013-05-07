#! /usr/bin/python

# Simple simulation of Max-weight scheduling for CDMA downlink.
# Rates for each user are static rates. No fading.

import numpy

slot=0
TOTAL_SLOTS=100000
NUM_USERS=10
# initial values close to zero.
T=[0.00001]*NUM_USERS
# link rates in packets per slot
R=[1]*NUM_USERS
# EWMA constant
t_C=20.0
# Queue size in packets
Q=[0]*NUM_USERS

while( slot < TOTAL_SLOTS ) :
  for i in range(0,NUM_USERS) :
    # update queue arrivals
    Q[i] = Q[i] + (i+1);
  current_and_historic = zip(T,R,Q);
  norm_rates = map( lambda pair : pair[2]/pair[0] , current_and_historic );
  user = numpy.argmax( norm_rates );
  print "Slot",slot,"user",user,"rate",R[user],"q",Q[user]
  # Update averages
  for i in range(0,NUM_USERS) :
    if   (i == user) :
      T[ i ] = (1-(1/t_C))*T[ i ] + (1/t_C)*R[ i ]
      Q[i] = Q[i] - R[i];
    elif (i != user) :
      T[ i ] = (1-(1/t_C))*T[ i ]
  slot = slot + 1;
