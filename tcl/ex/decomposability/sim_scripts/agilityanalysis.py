#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "resultslogarithmic/"

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# Auto-agility
linkspeed_range = logrange(1.0, 1000, 1000);
for linkspeed in linkspeed_range:
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeed1000x.plot", "a"),         (linkspeed,  ), 2, resultfolder + "1000x-link"+str(linkspeed), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeed100x.plot", "a"),          (linkspeed,  ), 2, resultfolder + "100x-link"+str(linkspeed), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeed10x.plot", "a"),           (linkspeed,  ), 2, resultfolder + "10x-link"+str(linkspeed), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeedcubicsfqCoDel.plot", "a"), (linkspeed,  ), 2, resultfolder + "cubicsfqCoDel-link"+str(linkspeed), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/linkspeedcubic.plot", "a"),         (linkspeed,  ), 2, resultfolder + "cubic-link"+str(linkspeed), "*");

# Cross-agility on delay
for delay in range(5, 151, 5):
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/delay1000x.plot", "a"),          (delay, ), 2, resultfolder + "1000x-delay"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/delay100x.plot", "a"),           (delay, ), 2, resultfolder + "100x-delay"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/delay10x.plot", "a"),            (delay, ), 2, resultfolder + "10x-delay"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/delaycubicsfqCoDel.plot", "a"),  (delay, ), 2, resultfolder + "cubicsfqCoDel-delay"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/delaycubic.plot", "a"),          (delay, ), 2, resultfolder + "cubic-delay"+str(delay), "*");

# Cross-agility on duty cycle
for onpercent in range(5, 101, 5):
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/dutycycle1000x.plot", "a"),         (onpercent, ), 2, resultfolder + "1000x-dutycycle"+str(onpercent), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/dutycycle100x.plot", "a"),          (onpercent, ), 2, resultfolder + "100x-dutycycle"+str(onpercent), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/dutycycle10x.plot", "a"),           (onpercent, ), 2, resultfolder + "10x-dutycycle"+str(onpercent), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/dutycyclecubicsfqCoDel.plot", "a"), (onpercent, ), 2, resultfolder + "cubicsfqCoDel-dutycycle"+str(onpercent), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/dutycyclecubic.plot", "a"),         (onpercent, ), 2, resultfolder + "cubic-dutycycle"+str(onpercent), "*");
