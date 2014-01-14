#! /usr/bin/python
import os
from analysis import *

# Set constants
iteration_count = 10
resultfolder = "resultssendersweep/"

# Cross-agility on num senders
for num_senders in range(1, 21, 1):
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders1000x.plot", "a"),         (num_senders, ),  num_senders, resultfolder + "1000x-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders100x.plot", "a"),          (num_senders, ),  num_senders, resultfolder + "100x-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senders10x.plot", "a"),           (num_senders, ),  num_senders, resultfolder + "10x-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senderscubicsfqCoDel.plot", "a"), (num_senders, ),  num_senders, resultfolder + "cubicsfqCoDel-num_senders"+str(num_senders), "*");
  plot_tpt_delay(range(1, iteration_count + 1), open("plots/num_senderscubic.plot", "a"),         (num_senders, ),  num_senders, resultfolder + "cubic-num_senders"+str(num_senders), "*");
