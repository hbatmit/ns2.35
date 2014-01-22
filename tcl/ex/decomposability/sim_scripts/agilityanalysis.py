#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "results2x/"

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# Auto-agility
linkspeed_range = logrange(1.0, 1000, 1000);
for linkspeed in linkspeed_range:
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeed2x.plot", "a"),           (linkspeed,  ), 2, resultfolder + "2x-link"+str(linkspeed), "*");
