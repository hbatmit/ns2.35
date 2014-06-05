#! /usr/bin/python
import os
import numpy
import math
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "results-rttvar/"

# Cross-agility on delay
for delay in range(5, 151, 5):
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/rtt150.plot", "a"),        (delay, ), 2, resultfolder + "rtt150"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/rtt140--160.plot", "a"),   (delay, ), 2, resultfolder + "rtt140--160"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/rtt110--200.plot", "a"),   (delay, ), 2, resultfolder + "rtt110--200"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/rtt50--250.plot", "a"),    (delay, ), 2, resultfolder + "rtt50--250"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/rtt10--280.plot", "a"),    (delay, ), 2, resultfolder + "rtt10--280"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/cubic.plot", "a"),         (delay, ), 2, resultfolder + "cubic"+str(delay), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("rttplots/cubicsfqCoDel.plot", "a"), (delay, ), 2, resultfolder + "cubicsfqCoDel"+str(delay), "*");
