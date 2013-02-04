#! /usr/bin/python
# Compute allocations of alpha-fair family of utility functions.

import sys
from math import log

def f_alpha(x,alpha):
  return x**(1-alpha)/(1-alpha) if alpha != 1 else log(x)

alpha=float(sys.argv[1])
for i in range(1,1000) :
  x=i/1000.0
  print x,"\t",f_alpha( 1*x, alpha )+f_alpha( 2*(1-x), alpha )
