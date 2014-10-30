// 
// config.hh     : Common defines + other config parameters
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: config.hh,v 1.8 2005/09/13 04:53:49 tomh Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

// Software information
#define PROGRAM "Diffusion 3.2.0"
#define RELEASE "Gear + Push Release"
#define DEFAULT_CONFIG_FILE "config.txt"

// Configurable parameters start here

// Change the following parameters to set how often interest messages
// are sent.
// To calculate the lambda, you should divide 1 by the
// number of milliseconds you want to have (on average) between
// interest messages. For example, 1 / 30000 (30 seconds) results in a
// lambda of 0.0000333333333.
#define INTEREST_LAMBDA   0.00003333333333333333 // 30 seconds bw interests
#define INTEREST_DELAY    30000                  // (msec) bw sends
#define ROUND_EXPIRATION  50000                  // (msec)

// This parameter specifies how often an exploratory message is allowed
// to go to the network. It's used to establish a reinforced path as well
// as discover new paths.
#define EXPLORATORY_DATA_DELAY    60      // seconds between sends
#define PUSH_EXPLORATORY_DELAY    60      // seconds between sends

// This parameter specifies how much time to wait before sending
// a positive reinforcement message in response to an exploratory
// data message.
#define POS_REINFORCEMENT_SEND_DELAY  600 // (msec) bw receive and send
#define POS_REINFORCEMENT_JITTER      200 // (msec) jitter

#ifdef WIRED
// These settings are for high-bandwidth point-to-point
// communication. In this case, we assume 10Mbit/s, a 1000-byte packet
// is sent in .8 msec.

// Use multiple sends when forwarding data
#undef USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY    100        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER    50        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY         50        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER        25        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY    50        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER   25        // (msec) jitter
#endif // WIRED

#ifdef USE_WINSNG2
// These settings are for the sensoria radios (20 kbits/s). Sending a
// 500-byte message takes 200 msec. Radios are TDMA for unicast and
// broadcast, therefore there should be no contention.

// Use multiple sends when forwarding data
#undef USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   1000        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER   750        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        150        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       100        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY   150        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER  100        // (msec) jitter
#endif // USE_WINSNG2

#ifdef USE_MOTE_NIC
// These settings are for the mote nic radios (10 kbits/s) and use
// S-MAC at 10% duty cycle. Sending a 500-byte packet takes about 400
// msec.

// Use single send when forwarding data
#define USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   1200        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER   600        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        800        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       400        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY   800        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER  400        // (msec) jitter
#endif // USE_MOTE_NIC

#ifdef USE_RPC
// These settings are for the Radiometrix RPC radios (13 kbits/s) with
// no CSMA, therefore we need to be more conservative. Sending a
// 500-byte packet takes about 400 msec.

//  Use single send when forwarding data
#define USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   2000        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER  1500        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY       1200        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       600        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY  1200        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER  600        // (msec) jitter
#endif // USE_RPC

#ifdef USE_SMAC
// These settings are for the mote nic radios (10 kbits/s) and use
// S-MAC at 10% duty cycle. Sending a 500-byte packet takes about 400
// msec.

// Use single send when forwarding data
#define USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   1200        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER   600        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        800        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       400        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY   800        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER  400        // (msec) jitter
#endif // USE_SMAC

#ifdef USE_EMSTAR
// These settings are for the mote nic radios (10 kbits/s) and use
// EMSTAR. This is a generic (conservative) setting for all
// emstar-class devices. This should be updated in the near future to
// allow device-specific configuration.

// Use single send when forwarding data
#define USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY   1200        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER   600        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY        800        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER       400        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY   800        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER  400        // (msec) jitter
#endif // USE_EMSTAR

#ifdef NS_DIFFUSION
// These settings are for high-bandwidth point-to-point
// communication. In this case, we assume 10Mbit/s, a 1000-byte packet
// is sent in .8 msec.

// Use multiple sends when forwarding data
#undef USE_BROADCAST_TO_MULTIPLE_RECIPIENTS

// Change the following parameters to set how long to wait between
// receiving an interest message from a neighbor and forwarding it.
#define INTEREST_FORWARD_DELAY    100        // (msec) bw receive and forward
#define INTEREST_FORWARD_JITTER    50        // (msec) jitter

// Change the following parameters to set how long to wait between
// receiving an exploratory data message from a neighbor and forwarding it.
#define DATA_FORWARD_DELAY         50        // (msec) bw receive and forward
#define DATA_FORWARD_JITTER        25        // (msec) jitter

#define PUSH_DATA_FORWARD_DELAY    50        // (msec) bw receive and forward
#define PUSH_DATA_FORWARD_JITTER   25        // (msec) jitter
#endif // NS_DIFFUSION

#ifndef INTEREST_FORWARD_DELAY
#error "No Radio Parameters Selected in ./configure !"
#endif // !INTEREST_FORWARD_DELAY

// The following timeouts are used for determining when gradients,
// subscriptions, filters and neighbors should expire. These are
// mostly a function of other parameters and should not be changed.
#define SUBSCRIPTION_TIMEOUT    (INTEREST_DELAY/1000 * 4)         // sec
#define GRADIENT_TIMEOUT        (INTEREST_DELAY/1000 * 5)         // sec
#define FILTER_TIMEOUT          (FILTER_KEEPALIVE_DELAY/1000 * 2) // sec
#define NEIGHBORS_TIMEOUT       (INTEREST_DELAY/1000 * 4)         // sec

// The following parameters are not dependent of radio properties
// and just specify how often diffusion/other modules check for events.
#define GRADIENT_DELAY          10000        // (msec) bw checks
#define REINFORCEMENT_DELAY     10000        // (msec) bw checks
#define SUBSCRIPTION_DELAY      10000        // (msec) bw checks
#define NEIGHBORS_DELAY         10000        // (msec) bw checks
#define FILTER_DELAY            60000        // (msec) bw checks
#define FILTER_KEEPALIVE_DELAY  20000        // (msec) bw sends

// Number of packets to keep in the hash table
#define HASH_TABLE_MAX_SIZE           1000
#define HASH_TABLE_REMOVE_AT_ONCE       20
#define HASH_TABLE_DATA_MAX_SIZE       100
#define HASH_TABLE_DATA_REMOVE_AT_ONCE  10
