// 
// message.cc    : Message and Filters' factories
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: message.cc,v 1.7 2005/09/13 04:53:49 tomh Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//  
//  Linking this file statically or dynamically with other modules is making
//  a combined work based on this file.  Thus, the terms and conditions of
//  the GNU General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of this file
//  give you permission to combine this file with free software programs or
//  libraries that are released under the GNU LGPL and with code included in
//  the standard release of ns-2 under the Apache 2.0 license or under
//  otherwise-compatible licenses with advertising requirements (or modified
//  versions of such code, with unchanged license).  You may copy and
//  distribute such a system following the terms of the GNU GPL for this
//  file and the licenses of the other code concerned, provided that you
//  include the source code of that other code when and as the GNU GPL
//  requires distribution of source code.
//
//  Note that people who make modified versions of this file are not
//  obligated to grant this special exception for their modified versions;
//  it is their choice whether to do so.  The GNU General Public License
//  gives permission to release a modified version without this exception;
//  this exception also makes it possible to release a modified version
//  which carries forward this exception.
//

#include "message.hh"

NRSimpleAttributeFactory<void *> ControlMsgAttr(CONTROL_MESSAGE_KEY,
						NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<void *> OriginalHdrAttr(ORIGINAL_HEADER_KEY,
						 NRAttribute::BLOB_TYPE);

Message * CopyMessage(Message *msg)
{
   Message *newMsg;

   newMsg = new Message(msg->version_, msg->msg_type_, msg->source_port_,
			msg->data_len_, msg->num_attr_, msg->pkt_num_,
			msg->rdm_id_, msg->next_hop_, msg->last_hop_);

   newMsg->new_message_ = msg->new_message_;
   newMsg->next_port_ = msg->next_port_;
   newMsg->msg_attr_vec_ = CopyAttrs(msg->msg_attr_vec_);

   return newMsg;
}
