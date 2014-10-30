/* Copyright (c) 2007 Sidney Doria
 * Federal University of Campina Grande
 * Brazil
 *
 * This file is part of PUMA.
 *
 *    PUMA is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published
 *    by the Free Software Foundation; either version 2 of the License,
 *    or(at your option) any later version.
 *
 *    PUMA is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with PUMA; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *    02110-1301 USA
 */
/*  
 *  PUMA was designed by Ravindra Vaishampayan and 
 *  J.J. Garcia-Luna-Aceves @ UCSC. 
 */
#ifndef PUMA_H_
#define PUMA_H_

#define SEED					0

#define NOW                             Scheduler::instance().clock()
#define NEVER                                  -1
#define INVALID_ADDRESS                 987654321
#define ALL_SHORTEST_PATHS                  false
#define PARTITION                               0
#define NORMAL                                  1
#define NUMBER_OF_PARENTS                       5
#define NUMBER_OF_ACK_TABLE_ENTRIES            30
#define MAX_HOP                                50
#define INVALID_DISTANCE                    65535
#define TENTATIVE_PARTITION             987654320

#define BROADCAST_JITTER                        0.010 //  10 milliseconds
#define ACK_TIMEOUT_INTERVAL                    0.040 //  40 milliseconds
#define MULTICAST_ANNOUNCEMENT_DELAY            0.150 // 150 milliseconds
#define PARTITION_ACK_TIMEOUT_INTERVAL          1.000 //   1 second
#define MULTICAST_ANNOUNCEMENT                  3.000 //   3 seconds
#define FLUSH_INTERVAL                      36000.000 // 600 minutes

/*
 * Message Cache
 */
struct message_item {
		nsaddr_t      source;
		u_int32_t     sequence;
		message_item* next;
};

class MessageCache {
protected:
		message_item* front;
		message_item* rear;
public:
		MessageCache();
		~MessageCache();
		bool find(nsaddr_t, u_int32_t);
		void add(nsaddr_t, u_int32_t);
		void remove_front();
};

/*
 * Connectivity List
 */
struct connectivity_item {
		nsaddr_t           next_hop;
		nsaddr_t           next_hops_next_hop;
		u_int16_t          distance_to_core;
		u_int32_t          sequence;
		double             last_ma_received;
		bool               mesh_member;
		connectivity_item* next;
};

class ConnectivityList {
protected:
		connectivity_item* front;
public:
		ConnectivityList();
		~ConnectivityList();
		void add(connectivity_item*);
		void add(nsaddr_t, u_int16_t, u_int32_t, nsaddr_t, bool);
		void remove(nsaddr_t);
		void remove_all();
		bool precedes(connectivity_item*, connectivity_item*);
		connectivity_item* get_front();
};

/*
 * Multicast Group List
 */
struct multicast_group {
		// Core independent fields. Don't change if core changes.
		nsaddr_t          multicast_address;
		bool              receiver;
		bool              mesh_member;
		bool              neighbor_confirmation_wait;
		bool              partition_confirmation_wait;
		bool              delayed_ma_pending;
		// Core dependent fields. Set to default if core changes.
		nsaddr_t          core_id;
		u_int32_t         last_sequence;
		double            last_ma_received;
		double            last_ma_originated;
		ConnectivityList* connectivity_list;
		multicast_group*  next;
};

class MulticastGroupList {
protected:
		multicast_group* front;
public:
		MulticastGroupList();
		~MulticastGroupList();
		multicast_group* find(nsaddr_t);
		void             add(multicast_group*);
		void             add(nsaddr_t, nsaddr_t);
		void             clear_group(multicast_group*);
		u_int32_t        size();
		multicast_group* get_front();
};

/*
 * Multicast Announcement
 */
struct multicast_announcement {
		nsaddr_t  multicast_address;
		nsaddr_t  core_id;
		nsaddr_t  next_hop;
		u_int32_t sequence;
		u_int16_t distance_to_core;
		bool      mesh_member;
};

/*
 * ACK Table
 */
struct ack_item {
		nsaddr_t heard_from;
		double   time_heard;
};

class AckTable {
protected:
		u_int32_t entries;
		u_int32_t empty_slot;
public:
		ack_item  ack_table_data[NUMBER_OF_ACK_TABLE_ENTRIES];
		AckTable();
		void      remove_obsolete_entries(double);
		u_int32_t first_occupied_slot();
		ack_item  element(u_int32_t);
		u_int32_t& get_entries();
		u_int32_t& get_empty_slot();
};

struct ack_timeout_message {
		nsaddr_t multicast_address;
		nsaddr_t next_hop;
};

/*
 * Routing Event
 */
enum EventType {
		SEND_NEXT_MA          = 11,
		SEND_NEXT_BUCKET      = 21,
		FLUSH_TABLES          = 31,
		SEND_PENDING_MA       = 41,
		JOIN_ACK_TIMEOUT      = 51,
		DATA_ACK_TIMEOUT      = 61,
		PARTITION_ACK_TIMEOUT = 71
};

class RoutingEvent : public Event {
public:
		RoutingEvent();
		virtual EventType get_type() = 0;
};

class SendNextMAEvent : public RoutingEvent {
protected:
		nsaddr_t multicast_address;
public:
		SendNextMAEvent(nsaddr_t);
		EventType get_type();
		nsaddr_t get_multicast_address();
};

class SendNextBucketEvent : public RoutingEvent {
public:
		SendNextBucketEvent();
		EventType get_type();
};

class FlushTablesEvent : public RoutingEvent {
public:
		FlushTablesEvent();
		EventType get_type();
};

class SendPendingMAEvent : public RoutingEvent {
protected:
		nsaddr_t multicast_address;
public:
		SendPendingMAEvent(nsaddr_t);
		EventType get_type();
		nsaddr_t get_multicast_address();
};

class JoinACKTimeoutEvent : public RoutingEvent {
protected:
		ack_timeout_message message;
public:
		JoinACKTimeoutEvent(ack_timeout_message);
		EventType get_type();
		ack_timeout_message get_message();
};

class DataACKTimeoutEvent : public RoutingEvent {
protected:
		ack_timeout_message message;
public:
		DataACKTimeoutEvent(ack_timeout_message);
		EventType get_type();
		ack_timeout_message get_message();
};

class PartitionACKTimeoutEvent : public RoutingEvent {
protected:
		ack_timeout_message message;
public:
		PartitionACKTimeoutEvent(ack_timeout_message);
		EventType get_type();
		ack_timeout_message get_message();
};

class PUMA;

/*
 * Routing Timer
 */
class RoutingTimer : public Handler {
protected:
		PUMA* agent;
public:
		RoutingTimer(PUMA*);
		virtual void handle(Event*);
};

/*
 * PUMA
 */
class PUMA : public Agent {
		friend class RoutingTimer;
protected:
		nsaddr_t           id;                   // Address of this node
		MessageCache       message_cache;        // To discard double packets
		MulticastGroupList multicast_group_list; // Multicast groups table
		u_int32_t          packet_sequence;      // Packet sequence number
		double             last_packet_received;
		RoutingTimer*      routing_timer;
		AckTable           ack_table;
		RNG                random;
		PortClassifier*    dmux_;                // Pass packets up to agents
		Trace*             logTarget_;           // For logging

        void      handle_join_from_transport(nsaddr_t);
        void      handle_leave_from_transport(nsaddr_t);
        void      handle_protocol_packet(Packet*);
        void      handle_data_from_transport(Packet*);
        void      reset_ma_timer(double);
        void      reset_bucket_timer(double);
        void      handle_ma(multicast_group*, multicast_announcement, nsaddr_t);
        void      send_next_ma(multicast_group*);
        void      send_a_single_ma(multicast_group*);
        void      send_a_generic_ma(multicast_group*, int, nsaddr_t);
        void      handle_bucket(Packet*);
        void      handle_data_packet_from_network(Packet*);
        void      accept_data_packet(Packet*);
        void      process_multicast_packet(Packet*);
        void      book_keeping_before_ma(multicast_group*);
        void      forward_fresh_data_packet(multicast_group*, Packet*);
        void      send_next_bucket();
        void      handle_partition_confirmation_request(multicast_group*,
			 											nsaddr_t, unsigned int);
        void      handle_fresher_ma(multicast_group* group,
				 					multicast_announcement ma, nsaddr_t source);
        void      fight_election(multicast_group*);
        void      fight_multiple_group_election_after_partition(
        		                                              multicast_group*);
        void      record_ack(nsaddr_t);
        void      send_a_partition_confirmation_request(multicast_group*);
        void      become_core(multicast_group*);
        void      become_receiver(multicast_group*);
        void      become_core_after_partition(multicast_group*);
        void      set_retransmission_timer(RoutingEvent*, double);
        void      send_a_delayed_ma(multicast_group*);
        void      update_group(multicast_group*, multicast_announcement,
                                  nsaddr_t);
        void      handle_protocol_event(Event*);
		void      routing_set_timer(RoutingEvent*, double);
        bool      should_i_send_ma(multicast_group*);
        bool      is_single_group_present();
        bool      do_i_forward_this_packet(multicast_group*, Packet*);
        bool      is_partition_flag_set(multicast_group*);
        bool      am_i_mesh_member(multicast_group*);
        bool      am_i_link_node(multicast_group*);
        bool      was_ack_received(nsaddr_t);
        u_int32_t get_number_of_mesh_children(connectivity_item*);
        u_int16_t distance_to_core(multicast_group*);
        nsaddr_t  get_next_hop(multicast_group*);
public:
		PUMA(nsaddr_t);
		int       command(int, const char*const*);
		void      recv(Packet*, Handler*);
};
#endif /*PUMA_H_*/
