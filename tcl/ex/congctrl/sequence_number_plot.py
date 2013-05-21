#! /usr/bin/python
import os
import sys
title=sys.argv[1]
os.system("grep \"^+\" cell-link.tr  | grep tcp | grep  -e \"0\.0 1\.0\" > first.seq")
os.system("grep \"^r\" cell-link.tr  | grep ack | grep  -e \"1\.0 0\.0\" > first.ack")
os.system("grep \"^d\" cell-link.tr  | grep tcp | grep  -e \"0\.0 1\.0\" > first.drops")
os.system("grep \"^+\" cell-link.tr  | grep tcp | grep  -e \"0\.1 2\.0\" > second.seq")
os.system("grep \"^r\" cell-link.tr  | grep ack | grep  -e \"2\.0 0\.1\" > second.ack")
os.system("grep \"^d\" cell-link.tr  | grep tcp | grep  -e \"0\.1 2\.0\" > second.drops")
os.system("gnuplot -p -e \"plot [0:*] 'first.seq'  u 2:11, 'first.ack'  u 2:11, 'first.drops'  u 2:11  \"")
os.system("gnuplot -p -e \"plot [0:*] 'second.seq' u 2:11, 'second.ack' u 2:11, 'second.drops' u 2:11  \"")
