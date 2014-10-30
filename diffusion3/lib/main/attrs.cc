//
// attrs.cc        : Attribute Functions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: attrs.cc,v 1.7 2005/09/13 04:53:49 tomh Exp $
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
// Linking this file statically or dynamically with other modules is making
// a combined work based on this file.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//
// In addition, as a special exception, the copyright holders of this file
// give you permission to combine this file with free software programs or
// libraries that are released under the GNU LGPL and with code included in
// the standard release of ns-2 under the Apache 2.0 license or under
// otherwise-compatible licenses with advertising requirements (or modified
// versions of such code, with unchanged license).  You may copy and
// distribute such a system following the terms of the GNU GPL for this
// file and the licenses of the other code concerned, provided that you
// include the source code of that other code when and as the GNU GPL
// requires distribution of source code.
//
// Note that people who make modified versions of this file are not
// obligated to grant this special exception for their modified versions;
// it is their choice whether to do so.  The GNU General Public License
// gives permission to release a modified version without this exception;
// this exception also makes it possible to release a modified version
// which carries forward this exception.
//

#include <stdlib.h>
#include <stdio.h>

#include "attrs.hh"

class PackedAttribute {
public:
  int32_t key_;
  int8_t type_;
  int8_t op_;
  int16_t len_;
  int32_t val_;
};

NRAttrVec * CopyAttrs(NRAttrVec *src_attrs)
{
  NRAttrVec::iterator itr;
  NRAttrVec *dst_attrs;
  NRAttribute *src;

  dst_attrs = new NRAttrVec;

  for (itr = src_attrs->begin(); itr != src_attrs->end(); ++itr){
    src = *itr;
    switch (src->getType()){
    case NRAttribute::INT32_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<int>(src->getKey(),
						NRAttribute::INT32_TYPE,
					        src->getOp(),
					       ((NRSimpleAttribute<int> *)src)->getVal()));
      break;

    case NRAttribute::FLOAT32_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<float>(src->getKey(),
						NRAttribute::FLOAT32_TYPE,
						src->getOp(),
						((NRSimpleAttribute<float> *)src)->getVal()));
      break;

    case NRAttribute::FLOAT64_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<double>(src->getKey(),
						 NRAttribute::FLOAT64_TYPE,
						 src->getOp(),
						 ((NRSimpleAttribute<double> *)src)->getVal()));
      break;

    case NRAttribute::STRING_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<char *>(src->getKey(),
						NRAttribute::STRING_TYPE,
						src->getOp(),
						((NRSimpleAttribute<char *> *)src)->getVal()));
      break;

    case NRAttribute::BLOB_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<void *>(src->getKey(),
					      NRAttribute::BLOB_TYPE,
					      src->getOp(),
					      ((NRSimpleAttribute<void *> *)src)->getVal(),
					      src->getLen()));
      break;

    default:

      DiffPrint(DEBUG_ALWAYS, "Unknown attribute type found in CopyAttrs() !\n");
      break;
    }
  }
  return dst_attrs;
}

void AddAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  NRAttrVec *copied_attrs;
  NRAttrVec::iterator itr;
  NRAttribute *src;

  if (attr_vec1 == NULL)
    attr_vec1 = new NRAttrVec;

  // Check if there's something to do
  if (attr_vec2 == NULL)
    return;

  // Duplicate source attributes so they can be added to the dst set
  copied_attrs = CopyAttrs(attr_vec2);

  // Add all attributes in the copied set into the dst set
  for (itr = copied_attrs->begin(); itr != copied_attrs->end(); ++itr){
    src = *itr;

    attr_vec1->push_back(src);
  }

  // Delete the copied_attr vector without deleting its attributes
  delete copied_attrs;
}

void ClearAttrs(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    delete *itr;
  }

  attr_vec->clear();
}

void PrintAttrs(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;
  NRAttribute *aux;
  int counter = 0;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    aux = *itr;
    DiffPrint(DEBUG_ALWAYS, "Attribute #%d\n", counter);
    counter++;
    DiffPrint(DEBUG_ALWAYS, "-------------\n");
    DiffPrint(DEBUG_ALWAYS, "Type = %d\n", aux->getType());
    DiffPrint(DEBUG_ALWAYS, "Key  = %d\n", aux->getKey());
    DiffPrint(DEBUG_ALWAYS, "Op   = %d\n", aux->getOp());
    DiffPrint(DEBUG_ALWAYS, "Len  = %d\n", aux->getLen());
    switch(aux->getType()){
    case NRAttribute::INT32_TYPE:
      DiffPrint(DEBUG_ALWAYS, "Val  = %d\n",
		((NRSimpleAttribute<int> *)aux)->getVal());
      break;
    case NRAttribute::FLOAT32_TYPE:
      DiffPrint(DEBUG_ALWAYS, "Val  = %f\n",
		((NRSimpleAttribute<float> *)aux)->getVal());
      break;
    case NRAttribute::FLOAT64_TYPE:
      DiffPrint(DEBUG_ALWAYS, "Val  = %f\n",
		((NRSimpleAttribute<double> *)aux)->getVal());
      break;
    case NRAttribute::STRING_TYPE:
      DiffPrint(DEBUG_ALWAYS, "Val  = %s\n",
		((NRSimpleAttribute<char *> *)aux)->getVal());
      break;
    case NRAttribute::BLOB_TYPE:
      DiffPrint(DEBUG_ALWAYS, "Val  = %s\n",
		((NRSimpleAttribute<void *> *)aux)->getVal());
      break;
    default:
      DiffPrint(DEBUG_ALWAYS, "Val  = Unknown\n");
      break;
    }
    DiffPrint(DEBUG_ALWAYS, "\n");
  }
}

DiffPacket AllocateBuffer(NRAttrVec *attr_vec)
{
  DiffPacket pkt;
  int len;

  len = CalculateSize(attr_vec);
  len = len + sizeof(struct hdr_diff);
  pkt = new int [1 + (len / sizeof(int))];

  if (pkt == NULL){
    DiffPrint(DEBUG_ALWAYS, "Cannot allocate memory for packet !\n");
    exit(-1);
  }

  return pkt;
}

int CalculateSize(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;
  NRAttribute *temp;
  int pad, attr_len;
  int total_len = 0;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    temp = *itr;
    attr_len = temp->getLen();

    // We have to pad to avoid bus errors
    pad = 0;
    if (attr_len % sizeof(int)){
      pad = sizeof(int) - (attr_len % sizeof(int));
    }
    total_len = total_len + sizeof(PackedAttribute) - sizeof(int32_t) + attr_len + pad;
  }
  return total_len;
}

int PackAttrs(NRAttrVec *attr_vec, char *start_pos)
{
  PackedAttribute *current;
  NRAttrVec::iterator itr;
  NRAttribute *temp;
  int32_t t;
  char *pos;
  int pad, attr_len;
  int total_len = 0;

  current = (PackedAttribute *) start_pos;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    temp = *itr;
    current->key_ = htonl(temp->getKey());
    current->type_ = temp->getType();
    current->op_ = temp->getOp();
    attr_len = temp->getLen();
    current->len_ = htons(attr_len);
    if (attr_len > 0){
      switch (current->type_){
      case NRAttribute::INT32_TYPE:

	t = htonl(((NRSimpleAttribute<int> *)temp)->getVal());
	memcpy(&current->val_, &t, sizeof(int32_t));

	break;

      default:

	memcpy(&current->val_, temp->getGenericVal(), attr_len);

	break;
      }
    }

    // We have to pad in order to avoid bus errors
    pad = 0;
    if (attr_len % sizeof(int)){
      pad = sizeof(int) - (attr_len % sizeof(int));
    }
    total_len = total_len + sizeof(PackedAttribute) - sizeof(int32_t) + attr_len + pad;
    pos = (char *) current + sizeof(PackedAttribute) - sizeof(int32_t) + attr_len + pad;
    current = (PackedAttribute *) pos;
  }
  return total_len;
}

NRAttrVec * UnpackAttrs(DiffPacket pkt, int num_attr)
{
  PackedAttribute *current;
  NRAttrVec *attr_vec;
  int attr_len;
  char *pos;
  int pad;

  attr_vec = new NRAttrVec;

  pos = (char *) &pkt[0];
  current = (PackedAttribute *) (pos + sizeof(struct hdr_diff));

  for (int i = 0; i < num_attr; i++){
    switch (current->type_){
    case NRAttribute::INT32_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<int>(ntohl(current->key_),
						NRAttribute::INT32_TYPE,
						current->op_,
						ntohl(*(int *)&current->val_)));
      break;

    case NRAttribute::FLOAT32_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<float>(ntohl(current->key_),
						NRAttribute::FLOAT32_TYPE,
					        current->op_,
					        *(float *)&current->val_));
      break;

    case NRAttribute::FLOAT64_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<double>(ntohl(current->key_),
					     NRAttribute::FLOAT64_TYPE,
					     current->op_,
					     *(double *)&current->val_));
      break;

    case NRAttribute::STRING_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<char *>(ntohl(current->key_),
					    NRAttribute::STRING_TYPE,
					    current->op_,
					    (char *)&current->val_));
      break;

    case NRAttribute::BLOB_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<void *>(ntohl(current->key_),
					  NRAttribute::BLOB_TYPE,
				          current->op_,
					  (void *)&current->val_,
			                  ntohs(current->len_)));
      break;

    default:

      DiffPrint(DEBUG_ALWAYS, "Unknown attribute type found in UnpackAttrs() !\n");
      break;
    }

    attr_len = ntohs(current->len_);

    pad = 0;
    if (attr_len % sizeof(int)){
      // We have to calculate how much 'padding' to skip in order to
      // avoid bus errors
      pad = sizeof(int) - (attr_len % sizeof(int));
    }
    pos = (char *) current + sizeof(PackedAttribute) - sizeof(int32_t) + attr_len + pad;
    current = (PackedAttribute *) pos;
  }

  return attr_vec;
}

bool PerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  if (attr_vec1->size() != attr_vec2->size())
    return false;

  if (OneWayPerfectMatch(attr_vec1, attr_vec2))
    if (OneWayPerfectMatch(attr_vec2, attr_vec1))
      return true;

  return false;
}

bool OneWayPerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  NRAttrVec::iterator itr1, itr2;
  NRAttribute *a;
  NRAttribute *b;

  for (itr1 = attr_vec1->begin(); itr1 != attr_vec1->end(); ++itr1){

    // For each attribute in vec1, we look in vec2 for the same attr
    a = *itr1;
    itr2 = attr_vec2->begin();
    b = a->find_matching_key_from(attr_vec2, itr2, &itr2);

    while (b){
      // They should have the same operator and be "EQUAL"
      if (a->getOp() == b->getOp())
	if (a->isEQ(b))
	  break;

      // Keys match but value/operator don't. Let's
      // Try to find another attribute with the same key
      itr2++;
      b = a->find_matching_key_from(attr_vec2, itr2, &itr2);
    }

    // Check if attribute found
    if (b == NULL)
      return false;
  }

  // All attributes match !
  return true;
}

bool MatchAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  if (OneWayMatch(attr_vec1, attr_vec2))
    if (OneWayMatch(attr_vec2, attr_vec1))
      return true;

  return false;
}

bool OneWayMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  NRAttrVec::iterator itr1, itr2;
  NRAttribute *a;
  NRAttribute *b;
  bool found_attr;

  for (itr1 = attr_vec1->begin(); itr1 != attr_vec1->end(); ++itr1){

    // For each attribute in vec1 that has an operator different
    // than "IS", we need to find a "matchable" one in vec2
    a = *itr1;
    if (a->getOp() == NRAttribute::IS)
      continue;

    itr2 = attr_vec2->begin();
    for (;;){
      b = a->find_matching_key_from(attr_vec2, itr2, &itr2);

      // Found ?
      if (!b)
	return false;

      // If Op is not "IS" we need keep looking
      if (b->getOp() != NRAttribute::IS){
	itr2++;
	continue;
      }

      // If a's Op is "EQ_ANY" that's enough   
      if (a->getOp() == NRAttribute::EQ_ANY){
	found_attr = true;
	break;
      }

      found_attr = false;

      switch (a->getOp()){

	 case NRAttribute::EQ:

	   if (b->isEQ(a))
	     found_attr = true;

	   break;

      case NRAttribute::GT:

	if (b->isGT(a))
	  found_attr = true;

	break;

      case NRAttribute::GE:

	if (b->isGE(a))
	  found_attr = true;

	break;

      case NRAttribute::LT:

	if (b->isLT(a))
	  found_attr = true;

	break;

      case NRAttribute::LE:

	if (b->isLE(a))
	  found_attr = true;

	break;

      case NRAttribute::NE:

	if (b->isNE(a))
	  found_attr = true;

	break;

      default:

	DiffPrint(DEBUG_ALWAYS, "Unknown operator found in OneWayMacth !\n");
	break;
      }

      if (found_attr)
	break;

      itr2++;
    }
  }

  // All attributes found !
  return true;
}

