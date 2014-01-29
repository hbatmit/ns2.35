#!  /usr/bin/python
import sys
fh = sys.stdin
choose = sys.argv[1]
tag    = sys.argv[2]
sum_tpts = 0.0
sum_dels = 0.0
count = 0
for line in fh:
  records = line.split()
  sender_id = records[0].split(":")[0]
  if (sender_id != choose and choose != "*"):
      continue
  sum_tpts += float( records[1].split("=")[1] )
  sum_dels += float( records[3].split("=")[1] )
  count += 1
print tag, sum_tpts / count, sum_dels / count
