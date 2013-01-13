
#
# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 

#
# Maintainer: Polly Huang <huang@isi.edu>
# Version Date: $Date: 1999/04/20 22:34:30 $
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/ex/trace-driven.tcl,v 1.1 1999/04/20 22:34:30 polly Exp $ (USC/ISI)
#
# An example script using a trace driven random variable

set dist [new RandomVariable/TraceDriven]
$dist set filename_ sample.trace
for {set i 0} {$i < 10} {incr i} {
    puts [$dist value]
}
