#
# Copyright (C) 2001 by USC/ISI
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
# Maintainer: Kun-chan Lan <kclan@isi.edu>
# Version Date: $Date: 2002/06/28 22:07:59 $
#
#
# simplified topology modified from isitopo.tcl
# used by simpleweb.tcl
#
#         ISI hosts      remote hosts
#
#      [ISI clients]
#                        
#            1---0---2
#        server     client
#

proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {

       $ns duplex-link $n1 $n2 $bw $delay $queue_method
       $ns queue-limit $n1 $n2 $queue_length
       $ns queue-limit $n2 $n1 $queue_length
}

proc create_topology {} {
global ns n verbose num_node num_nonisi_web_client num_isi_server num_ftp_client

set num_node 3
set queue_method RED
set queue_length 50


for {set i 0} {$i < $num_node} {incr i} {
	set n($i) [$ns node]
}

my-duplex-link $ns $n(0) $n(1) 100Mb 1ms $queue_method $queue_length
my-duplex-link $ns $n(0) $n(2) 100Mb 1ms $queue_method $queue_length

}
# end of create_topology

