# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License,
#  version 2, as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
#
#  The copyright of this module includes the following
#  linking-with-specific-other-licenses addition:
#
#  In addition, as a special exception, the copyright holders of
#  this module give you permission to combine (via static or
#  dynamic linking) this module with free software programs or
#  libraries that are released under the GNU LGPL and with code
#  included in the standard release of ns-2 under the Apache 2.0
#  license or under otherwise-compatible licenses with advertising
#  requirements (or modified versions of such code, with unchanged
#  license).  You may copy and distribute such a system following the
#  terms of the GNU GPL for this module and the licenses of the
#  other code concerned, provided that you include the source code of
#  that other code when and as the GNU GPL requires distribution of
#  source code.
#
#  Note that people who make modified versions of this module
#  are not obligated to grant this special exception for their
#  modified versions; it is their choice whether to do so.  The GNU
#  General Public License gives permission to release a modified
#  version without this exception; this exception also makes it
#  possible to release a modified version which carries forward this
#  exception.
# 
#	Author:		Kannan Varadhan	<kannan@isi.edu>
#	Version Date:	Mon Jun 30 15:51:33 PDT 1997
#
# @(#) $Header: /cvsroot/nsnam/ns-2/tcl/mcast/srm-nam.tcl,v 1.3 2005/09/16 03:05:44 tomh Exp $ (USC/ISI)
#

if ![info exists ctrlFid] {
    set ctrlFid 39
}

set sessFid [incr ctrlFid]
set rqstFid [incr ctrlFid]
set reprFid [incr ctrlFid]

Agent/SRM instproc send {type args} {
#    eval $self evTrace $proc $type $args
    global sessFid rqstFid reprFid

    set fid [$self set fid_]
    switch $type {
    session	{ $self set fid_ $sessFid }
    request	{ $self set fid_ $rqstFid }
    repair	{ $self set fid_ $reprFid }
    }
    eval $self cmd send $type $args
    $self set fid_ $fid
}
