
/*
 * diff_sink.h
 * Copyright (C) 2000 by the University of Southern California
 * $Id: diff_sink.h,v 1.5 2005/08/25 18:58:03 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

/*****************************************************************/
/*  diff_sink.h : Header file for sink agent                     */
/*  By     : Chalermek Intanagonwiwat (USC/ISI)                  */
/*  Last Modified : 8/21/99                                      */
/*****************************************************************/


#ifndef ns_diff_sink_h
#define ns_diff_sink_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "diff_header.h"
#include "diffusion.h"


class SinkAgent;

// Timer for packet rate

class Sink_Timer : public TimerHandler {
 public:
	Sink_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// For periodic report of avg and var pkt received.

class Report_Timer : public TimerHandler {
 public:
	Report_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// Timer for periodic interest

class Periodic_Timer : public TimerHandler {
 public:
	Periodic_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// Class SinkAgent as source and sink for directed diffusion

class SinkAgent : public Agent {

 public:
  SinkAgent();
  int command(int argc, const char*const* argv);
  virtual void timeout(int);

  void report();
  void recv(Packet*, Handler*);
  void reset();
  void set_addr(ns_addr_t);
  int get_pk_count();
  void incr_pk_count();
  Packet *create_packet();

 protected:
  bool APP_DUP_;

  bool periodic_;
  bool always_max_rate_;
  int pk_count;
  unsigned int data_type_;
  int num_recv;
  int num_send;
  int RecvPerSec;

  double cum_delay;
  double last_arrival_time;

  Data_Hash_Table DataTable;

  void Terminate();
  void bcast_interest();
  void data_ready();
  void start();
  void stop();
  virtual void sendpkt();

  int running_;
  int random_;
  int maxpkts_;
  double interval_;

  int simple_report_rate;
  int data_counter;
 
  Sink_Timer sink_timer_;
  Periodic_Timer periodic_timer_;
  Report_Timer report_timer_;

  friend class Periodic_Timer;
};


#endif











