#!/bin/sh
# To remove temporary files in doc.
# To run: "./remove.com"
#
rm -f *.aux *.log *.dvi
