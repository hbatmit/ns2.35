#! /usr/bin/python

# Simple simulation of Proportional Fair Scheduler for CDMA downlink.
# Rates for each user are static rates. No fading.

import numpy

slot=0
TOTAL_SLOTS=100000
NUM_USERS=10
T=[0.00001]*NUM_USERS
R=[120.0,240.0,360.0,480.0,600.0,720.0,840.0,960.0,1080.0,1200.0]
t_C=20.0;

while( slot < TOTAL_SLOTS ) :
  current_and_historic = zip(T,R);
  norm_rates = map( lambda pair : pair[1]/pair[0] , current_and_historic );
  user = numpy.argmax( norm_rates );
  print "Slot",slot,"user",user,"rate",R[user]
  # Update averages
  for i in range(0,NUM_USERS) :
    if   (i == user) :
      T[ i ] = (1-1/t_C)*T[ i ] + 1/t_C*R[ i ]
    elif (i != user) :
      T[ i ] = (1-1/t_C)*T[ i ]
  slot = slot + 1;
