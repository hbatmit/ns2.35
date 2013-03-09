#! /usr/bin/python
import sys

# Get file name using cat or the like
fh=sys.stdin

# Get all source file paths relative to ns_root
src_file_paths=sys.argv[1:len(sys.argv)];

# Read the dep.txt file
for line in fh.readlines() :
  line.strip()
  records=line.split()

  # potentially the target file name
  target_record=records[0]

  # Check if there is a target
  if ( target_record.find(":") == -1 ) :
      print line,

  # There is a target, split it apart
  else :
      # target_file name (g++ -MM removes directories)
      target_file=target_record.split(":")[0]

      # Get file alone without the suffix
      target=target_file.split(".")[0]

      # look through all source files you have
      for src_file_path in src_file_paths :
         # get .cc file name
         src_file = src_file_path.split("/")[-1];
         # get name of src without .cc suffix
         src=src_file.split(".")[0];
         # compare against the target
         if (src == target) :
            line=line.replace(target,src_file_path.split(".")[0],1)
            print line,
            break;
