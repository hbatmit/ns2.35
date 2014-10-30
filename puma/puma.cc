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

#include <classifier-port.h>
#include <agent.h>
#include <ip.h>
#include <packet.h>
#include <timer-handler.h>
#include <address.h>
#include <rng.h>
#include <cmu-trace.h>
#include <trace.h>

#include "puma_pkt.h"
#include "puma.h"

/*
 * Message Cache
 */
MessageCache::MessageCache() {
		front = NULL;
		rear  = NULL;
}

MessageCache::~MessageCache() {
		message_item* temp;
		while (front != NULL) {
			temp = front->next;
			delete front;
			front = temp;
		}
}

bool
MessageCache::find(nsaddr_t source, u_int32_t sequence) {
        for (message_item* temp = front; temp != NULL; temp = temp->next)
        	if ((temp->source == source) && (temp->sequence == sequence))
        		return true;
        return false;
}

void
MessageCache::add(nsaddr_t source, u_int32_t sequence) {
		if (front == NULL) {
			rear  = new message_item();
			front = rear;
		}
		else {
			rear->next = new message_item();
			rear = rear->next;
		}
		rear->source   = source;
		rear->sequence = sequence;
		rear->next     = NULL;
}

void
MessageCache::remove_front() {
		if (front != NULL) {
            message_item* temp = front;
            front = front->next;
            delete temp;
            if (front == NULL)
            	rear = NULL;
        }
}

/*
 * Connectivity List
 */
ConnectivityList::ConnectivityList() {
		front = NULL;
}

ConnectivityList::~ConnectivityList() {
		connectivity_item* temp;
		while (front != NULL) {
			temp = front->next;
			delete front;
			front = temp;
		}
}

bool
ConnectivityList::precedes(connectivity_item* item1, connectivity_item* item2) {
		if (item1->sequence > item2->sequence)
			return true;
		else if (item1->sequence < item2->sequence)
			return false;
		else if (item1->distance_to_core < item2->distance_to_core)
			return true;
		else if (item1->distance_to_core > item2->distance_to_core)
			return false;
		else if (item1->next_hop > item2->next_hop)
			return true;
		else
			return false;
}

void
ConnectivityList::add(connectivity_item* item) {
		if (item != NULL) {
			remove(item->next_hop);
			if (front == NULL) {
				item->next = NULL;
				front = item;
			}
			else if (precedes(item, front)) {
				item->next = front;
				front = item;
			}
			else {
				connectivity_item* temp = front;
				while (temp->next != NULL) {
					if (precedes(item, temp->next))
						break;
					temp = temp->next;
				}
				item->next = temp->next;
				temp->next = item;
			}
		}
}

void
ConnectivityList::add(nsaddr_t next_hop, u_int16_t distance_to_core,
		              u_int32_t sequence, nsaddr_t next_hops_next_hop,
		              bool mesh_member){
		connectivity_item* temp = new connectivity_item();
		temp->next_hop = next_hop;
		temp->sequence = sequence;
		temp->distance_to_core = distance_to_core;
		temp->last_ma_received = NOW;
		temp->next_hops_next_hop = next_hops_next_hop;
		temp->mesh_member = mesh_member;
		add(temp);
}

void
ConnectivityList::remove(nsaddr_t next_hop) {
		if (front != NULL) {
			connectivity_item* temp = front;
			if (front->next_hop == next_hop) {
				front = front->next;
				delete temp;
				return;
			}
			while (temp->next != NULL) {
				if (temp->next->next_hop == next_hop) {
					connectivity_item* to_be_freed = temp->next;
					temp->next = temp->next->next;
					delete to_be_freed;
					return;
				}
				temp = temp->next;
			}
		}
}

connectivity_item*
ConnectivityList::get_front() {
	return front;
}

/*
 * Multicast Groups
 */
MulticastGroupList::MulticastGroupList() {
		front = NULL;
}

MulticastGroupList::~MulticastGroupList() {
		multicast_group* temp;
		while (front != NULL) {
			temp = front->next;
			delete front;
			front = temp;
		}
}

multicast_group*
MulticastGroupList::find(nsaddr_t group) {
		multicast_group* temp = front;
		while (temp != NULL) {
			if (temp->multicast_address == group)
				return temp;
			temp = temp->next;
		}
		return NULL;
}

void
MulticastGroupList::add(multicast_group* group) {
		if (group != NULL) {
			group->next = front;
			front = group;
		}
}

void
MulticastGroupList::add(nsaddr_t multicast_address, nsaddr_t core_id) {
		multicast_group* temp = new multicast_group();
		temp->multicast_address           = multicast_address;
		temp->receiver                    = FALSE;
		temp->mesh_member                 = FALSE;
		temp->neighbor_confirmation_wait  = FALSE;
		temp->partition_confirmation_wait = FALSE;
		temp->delayed_ma_pending          = FALSE;
		temp->last_sequence               = 0;
		temp->core_id                     = core_id;
		temp->last_ma_received            = NEVER;
		temp->last_ma_originated          = NEVER;
		temp->connectivity_list           = new ConnectivityList();
		temp->next                        = NULL;
		add(temp);
}

void
MulticastGroupList::clear_group(multicast_group* group) {
		if (group != NULL) {
			group->last_ma_originated = NEVER;
			group->neighbor_confirmation_wait = FALSE;
			group->partition_confirmation_wait = FALSE;
			group->delayed_ma_pending = FALSE;
			group->last_sequence = 0;
			group->core_id = INVALID_ADDRESS;
			group->last_ma_received = NEVER;
			group->mesh_member = FALSE;
			delete group->connectivity_list;
			group->connectivity_list = new ConnectivityList();
		}
}

u_int32_t
MulticastGroupList::size() {
		u_int32_t i = 0;
		multicast_group* temp = front;
		while (temp != NULL) {
			i++;
			temp = temp->next;
		}
        return i;
}

multicast_group*
MulticastGroupList::get_front() {
		return front;
}

/*
 * ACK Table
 */
AckTable::AckTable() {
		entries    = 0;
		empty_slot = 0;
}

void
AckTable::remove_obsolete_entries(double current_time) {
		u_int32_t i;
		i = first_occupied_slot();
		while((current_time - element(i).time_heard) >
				ACK_TIMEOUT_INTERVAL && entries > 0) {
			(entries)--;
			i++;
			i%=NUMBER_OF_ACK_TABLE_ENTRIES;
		}
}

u_int32_t
AckTable::first_occupied_slot() {
		u_int32_t first_occupied_slot;
		if (entries <= empty_slot)
			first_occupied_slot = empty_slot - entries;
		else
			first_occupied_slot = NUMBER_OF_ACK_TABLE_ENTRIES +
									empty_slot - entries;
		return(first_occupied_slot);
}

ack_item
AckTable::element(u_int32_t index) {
		return ack_table_data[index];
}

u_int32_t&
AckTable::get_entries() {
		return entries;
}

u_int32_t&
AckTable::get_empty_slot() {
		return empty_slot;
}

/*
 * Routing Event
 */
RoutingEvent::RoutingEvent() : Event() {
}

/*
 * Send Next MA Event
 */
SendNextMAEvent::SendNextMAEvent(nsaddr_t multicast_address) {
		this->multicast_address = multicast_address;
}

EventType
SendNextMAEvent::get_type() {
		return SEND_NEXT_MA;
}

nsaddr_t
SendNextMAEvent::get_multicast_address() {
		return multicast_address;
}

/*
 * Send Next Bucket Event
 */
SendNextBucketEvent::SendNextBucketEvent() {
}

EventType
SendNextBucketEvent::get_type() {
		return SEND_NEXT_BUCKET;
}

/*
 * Flush Tables Event
 */
FlushTablesEvent::FlushTablesEvent() {
}

EventType
FlushTablesEvent::get_type() {
		return FLUSH_TABLES;
}

/*
 * Send Pending MA Event
 */
SendPendingMAEvent::SendPendingMAEvent(nsaddr_t multicast_address) {
		this->multicast_address = multicast_address;
}

EventType
SendPendingMAEvent::get_type() {
		return SEND_PENDING_MA;
}

nsaddr_t
SendPendingMAEvent::get_multicast_address() {
		return multicast_address;
}

/*
 * Join ACK Timeout Event
 */
JoinACKTimeoutEvent::JoinACKTimeoutEvent(ack_timeout_message message) {
		this->message = message;
}

EventType
JoinACKTimeoutEvent::get_type() {
		return JOIN_ACK_TIMEOUT;
}

ack_timeout_message
JoinACKTimeoutEvent::get_message() {
		return message;
}

/*
 * Data ACK Timeout Event
 */
DataACKTimeoutEvent::DataACKTimeoutEvent(ack_timeout_message message) {
		this->message = message;
}

EventType
DataACKTimeoutEvent::get_type() {
		return DATA_ACK_TIMEOUT;
}

ack_timeout_message
DataACKTimeoutEvent::get_message() {
		return message;
}

/*
 * Partition ACK Timeout Event
 */
PartitionACKTimeoutEvent::PartitionACKTimeoutEvent(ack_timeout_message message){
		this->message = message;
}

EventType
PartitionACKTimeoutEvent::get_type() {
		return PARTITION_ACK_TIMEOUT;
}

ack_timeout_message
PartitionACKTimeoutEvent::get_message() {
		return message;
}

/*
 * Routing Timer
 */
RoutingTimer::RoutingTimer(PUMA* agent) : Handler() {
		this->agent = agent;
}

void
RoutingTimer::handle(Event* e) {
		agent->handle_protocol_event(e);
}

/*
 * TCL Hooks
 */
int hdr_puma::offset_;
static class PUMAHeaderClass : public PacketHeaderClass {
public:
		PUMAHeaderClass() : PacketHeaderClass("PacketHeader/PUMA",
												sizeof(hdr_puma)) {
			bind_offset(&hdr_puma::offset_);
		}
} class_rtProtoPUMA_hdr;

static class PUMAClass : public TclClass {
public:
		PUMAClass() : TclClass("Agent/PUMA") {}
		TclObject* create(int argc, const char*const* argv) {
			return (new PUMA((nsaddr_t) Address::instance().str2addr(argv[4])));
		}
} class_rtProtoPUMA;

/*
 * PUMA Agent
 */
PUMA::PUMA(nsaddr_t new_id) : Agent(PT_PUMA), message_cache() {
	id                   = new_id;
	packet_sequence      = 0;
	last_packet_received = NEVER;
	logTarget_           = NULL;
	routing_timer        = new RoutingTimer(this);
	random.set_seed(long(SEED));
	routing_set_timer(new SendNextBucketEvent(),
			                      random.uniform(1.0) * MULTICAST_ANNOUNCEMENT);
}

int
PUMA::command(int argc, const char*const* argv) {
        if (argc == 2) {
            Tcl& tcl = Tcl::instance();
            if (strncasecmp(argv[1], "start", 2) == 0) {
                return TCL_OK;
            }
            if (strncasecmp(argv[1], "id", 2) == 0) {
                tcl.resultf("%d", id);
                return TCL_OK;
            }
        } else if (argc == 3) {
            if (strcmp(argv[1], "log-target")  == 0 ||
                strcmp(argv[1], "tracetarget") == 0) {
                logTarget_ = (Trace*)TclObject::lookup(argv[2]);
                if (logTarget_ == NULL)
                    return TCL_ERROR;
                return TCL_OK;
            }
            if (strcmp(argv[1], "port-dmux") == 0) {
                dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
                if (dmux_ == NULL)
                    return TCL_ERROR;
                return TCL_OK;
            }
            if (strcmp(argv[1], "join") == 0) {
            	handle_join_from_transport(atoi(argv[2]));
                return TCL_OK;
            }
            if (strcmp(argv[1], "leave") == 0) {
                handle_leave_from_transport(atoi(argv[2]));
                return TCL_OK;
            }
        }
        return Agent::command(argc, argv);
}

void
PUMA::recv(Packet* p, Handler*) {
		hdr_cmn *ch = HDR_CMN(p);
		hdr_ip  *ih = HDR_IP(p);
		if ((HDR_CMN(p)->ptype() == PT_PUMA))
			handle_protocol_packet(p);
		else if ((ih->saddr() == id) && (ch->num_forwards() == 0))
			handle_data_from_transport(p);
		else
			handle_data_packet_from_network(p);
}

void
PUMA::set_retransmission_timer(RoutingEvent* event, double timeout_interval){
        routing_set_timer(event, timeout_interval);
}

void
PUMA::routing_set_timer(RoutingEvent* event, double delay) {
        Scheduler::instance().schedule(routing_timer, event, delay);
}

void
PUMA::handle_protocol_event(Event* e) {
        if (e != NULL) {
            RoutingEvent* re = (RoutingEvent*)e;
            switch (re->get_type())     {
				case SEND_NEXT_MA: {
					SendNextMAEvent* event = (SendNextMAEvent*)re;
					multicast_group* group =
				      multicast_group_list.find(event->get_multicast_address());
					if (should_i_send_ma(group))
						send_next_ma(group);
					break;
				}
                case SEND_NEXT_BUCKET: {
                    send_next_bucket();
                    break;
                }
                case FLUSH_TABLES: {
                    message_cache.remove_front();
                    break;
                }
                case SEND_PENDING_MA: {
					SendPendingMAEvent* event = (SendPendingMAEvent*)re;
					multicast_group* group =
				      multicast_group_list.find(event->get_multicast_address());
                    send_a_single_ma(group);
                    group->delayed_ma_pending = false;
                    break;
                }
                case JOIN_ACK_TIMEOUT: {
					JoinACKTimeoutEvent* event = (JoinACKTimeoutEvent*)re;
                    ack_timeout_message message = event->get_message();
                    multicast_group* group =
                         multicast_group_list.find(message.multicast_address);
                    fight_election(group);
                    break;
                }
                case DATA_ACK_TIMEOUT: {
					DataACKTimeoutEvent* event = (DataACKTimeoutEvent*)re;
                    ack_timeout_message message = event->get_message();
                    multicast_group* group =
                         multicast_group_list.find(message.multicast_address);
                    if (!(was_ack_received(message.next_hop)))
                        group->connectivity_list->remove(message.next_hop);
                    break;
                }
                case PARTITION_ACK_TIMEOUT: {
					PartitionACKTimeoutEvent* event =
							                      (PartitionACKTimeoutEvent*)re;
                    ack_timeout_message message = event->get_message();
                    multicast_group* group =
                    	 multicast_group_list.find(message.multicast_address);
                    fight_multiple_group_election_after_partition(group);
                    break;
                }
            }
            delete re;
        }
}

bool
PUMA::was_ack_received(nsaddr_t destination) {
        u_int32_t i, j;
        ack_table.remove_obsolete_entries(NOW);
        i = ack_table.first_occupied_slot();
        for (j = 0; j < ack_table.get_entries(); j++) {
            if ((ack_table.element(i)).heard_from == destination)
                return true;
            i++;
            i%=NUMBER_OF_ACK_TABLE_ENTRIES;
        }
        return false;
}

bool
PUMA::should_i_send_ma(multicast_group* group) {
       if (group->last_ma_originated == NEVER)
           return true;
       if (floor(((NOW - group->last_ma_originated)*1000+0.5)/1000) >=
MULTICAST_ANNOUNCEMENT )
           return true;
       return false;
}

void
PUMA::accept_data_packet(Packet* p) {
        dmux_->recv(p, (Handler*)0);
}

void
PUMA::handle_protocol_packet(Packet* p) {
        hdr_puma *ph = HDR_PUMA(p);
        if (ph->ma_count > 0)
            handle_bucket(p);
        else
            handle_data_packet_from_network(p);
};

void
PUMA::handle_data_packet_from_network(Packet* p) {
        hdr_cmn   *ch = HDR_CMN(p);
        hdr_ip    *ih = HDR_IP(p);
        multicast_group* group = multicast_group_list.find(ih->daddr());
        record_ack(ch->prev_hop_);
        if (group != NULL && ih->saddr() != id  &&
            !(message_cache.find(ih->saddr(), ch->uid())) && ih->ttl() > 0) {
            if (am_i_mesh_member(group)) {
                ch->prev_hop_ = id;
                process_multicast_packet(p);
            }
            else if (do_i_forward_this_packet(group, p)) {
                ch->prev_hop_ = id;
                forward_fresh_data_packet(group, p);
                ack_timeout_message message;
                message.multicast_address = group->multicast_address;
                message.next_hop =
                		        group->connectivity_list->get_front()->next_hop;
                set_retransmission_timer(new DataACKTimeoutEvent(message),
                                         DATA_ACK_TIMEOUT);
            } else
                Packet::free(p);
        }
        else
            Packet::free(p);
}

void
PUMA::process_multicast_packet(Packet* p) {
        hdr_cmn   *ch = HDR_CMN(p);
        hdr_ip    *ih = HDR_IP(p);
        multicast_group* group = multicast_group_list.find(ih->daddr());
        (ih->ttl())--;
        if (group->receiver)
            accept_data_packet(p->copy());
        message_cache.add(ih->saddr(), ch->uid());
        routing_set_timer(new FlushTablesEvent(), FLUSH_INTERVAL);
        ch->direction() = hdr_cmn::DOWN;
        ch->addr_type() = NS_AF_INET;
        ch->next_hop()  = IP_BROADCAST;
        Scheduler::instance().schedule(target_, p,
                                       BROADCAST_JITTER * random.uniform(1.0));
}

void
PUMA::fight_multiple_group_election_after_partition(multicast_group* group) {
        if (!(group->partition_confirmation_wait)) {
            group->partition_confirmation_wait = true;
            send_a_partition_confirmation_request(group);
            ack_timeout_message message;
            message.multicast_address = group->multicast_address;
            message.next_hop = INVALID_ADDRESS;
            set_retransmission_timer(new PartitionACKTimeoutEvent(message),
                                     PARTITION_ACK_TIMEOUT_INTERVAL);
        }
        else {
            group->partition_confirmation_wait = false;
            if (is_partition_flag_set(group) && group->receiver)
                become_core_after_partition(group);
	}
}

void
PUMA::fight_election(multicast_group* group) {
        if (!(group->neighbor_confirmation_wait)) {
            group->neighbor_confirmation_wait = true;
            send_a_single_ma(group);
            ack_timeout_message message;
            message.multicast_address = group->multicast_address;
            message.next_hop = INVALID_ADDRESS;
            set_retransmission_timer(new JoinACKTimeoutEvent(message),
                                     ACK_TIMEOUT_INTERVAL);
        }
        else {
            group->neighbor_confirmation_wait = false;
            if (group->core_id == INVALID_ADDRESS) {
                group->receiver = true;
                become_core(group);
            }
            else {
                become_receiver(group);
            }
        }
}

void
PUMA::handle_join_from_transport(nsaddr_t group_address) {
        multicast_group* group = multicast_group_list.find(group_address);
        if (group == NULL) {
            multicast_group_list.add(group_address, INVALID_ADDRESS);
            group = multicast_group_list.find(group_address);
            fight_election(group);
        } else if (group->core_id == INVALID_ADDRESS)
            fight_election(group);
        else
            become_receiver(group);
}

void
PUMA::become_receiver(multicast_group* group) {
        if (!(group->receiver)) {
            group->receiver = true;
            if (!(group->mesh_member))
                send_a_single_ma(group);
        }
}

bool
PUMA::am_i_mesh_member(multicast_group* group) {
        if (group->receiver)
            return true;
        else if (am_i_link_node(group))
            return true;
        else
            return false;
}

bool
PUMA::am_i_link_node(multicast_group* group) {
        if (group == NULL)
            return false;
        else {
            if (get_number_of_mesh_children(
            		                group->connectivity_list->get_front()) == 0)
                return false;
            else
                return true;
        }
}

bool
PUMA::is_partition_flag_set(multicast_group* group) {
        if (group->last_ma_received == NEVER ||
            ((NOW  - group->last_ma_received) <
             (3  * MULTICAST_ANNOUNCEMENT)))
            return false;
        else
            return true;
}

void
PUMA::update_group(multicast_group* group, multicast_announcement ma,
                       nsaddr_t source) {
        multicast_group_list.clear_group(group);
        group->last_sequence = ma.sequence;
        group->core_id = ma.core_id;
        group->last_ma_received = NOW;
        group->connectivity_list->add(source, ma.distance_to_core + 1,
                ma.sequence, ma.next_hop, ma.mesh_member);
}

void
PUMA::handle_leave_from_transport(nsaddr_t group_address) {
        multicast_group* group = multicast_group_list.find(group_address);
        if (group != NULL && group->receiver) {
            group->receiver = false;
            if (group->core_id == id)
                multicast_group_list.clear_group(group);
        }
}

void
PUMA::become_core(multicast_group* group) {
        group->core_id = id;
        send_next_ma(group);
}

void
PUMA::become_core_after_partition(multicast_group* group) {
        multicast_group_list.clear_group(group);
        group->core_id = id;
        send_next_ma(group);
}

void
PUMA::forward_fresh_data_packet(multicast_group* group, Packet* p) {
        hdr_cmn   *ch = HDR_CMN(p);
        hdr_ip    *ih = HDR_IP(p);
        if (group->connectivity_list->get_front() == NULL) {
            send_a_single_ma(group);
            Packet::free(p);
        }
        else {
            ch->direction() = hdr_cmn::DOWN;
            ch->next_hop()  = IP_BROADCAST;
            (ih->ttl())--;
            Scheduler::instance().schedule(target_, p,
                                       BROADCAST_JITTER * random.uniform(1.0));
            message_cache.add(ih->saddr(),ch->uid());
            routing_set_timer(new FlushTablesEvent(), FLUSH_INTERVAL);
        }
}

u_int32_t
PUMA::get_number_of_mesh_children(connectivity_item* front) {
        connectivity_item* temp = front;
        u_int32_t number_of_children = 0;
        while (temp != NULL) {
            if (
                (temp->mesh_member) &&

                ((temp->distance_to_core - 1) >
                (front->distance_to_core)) &&

                ((NOW - temp->last_ma_received) <=
                (2 * MULTICAST_ANNOUNCEMENT)) &&

                ((ALL_SHORTEST_PATHS) ||
                 (temp->next_hops_next_hop <= id))
               )
                number_of_children++;
            temp = temp->next;
        }
        return number_of_children;
}

bool
PUMA::do_i_forward_this_packet(multicast_group* group, Packet* p) {
        hdr_cmn* ch = HDR_CMN(p);
        connectivity_item* temp = group->connectivity_list->get_front();
        while (temp != NULL) {
            if (ch->prev_hop_ == temp->next_hop)
                if (temp->last_ma_received != NEVER &&
                    ((NOW - temp->last_ma_received) <=
                    (2 * MULTICAST_ANNOUNCEMENT)) &&
                    temp->next_hops_next_hop == id)
                    return true;
                else
                    return false;
            temp = temp->next;
	    }
        return false;
}

void
PUMA::handle_data_from_transport(Packet* p) {
        hdr_cmn   *ch = HDR_CMN(p);
        hdr_ip    *ih = HDR_IP(p);
        hdr_puma  *ph = HDR_PUMA(p);
        multicast_group* group = multicast_group_list.find(ih->daddr());
        if (group == NULL) {
            multicast_group_list.add(ih->daddr(), INVALID_ADDRESS);
            group = multicast_group_list.find(ih->daddr());
        }
        packet_sequence++;
        ch->direction() = hdr_cmn::DOWN;
        ch->size()      += IP_HDR_LEN;
        ch->error()     = 0;
        ch->prev_hop_   = id;
        ch->next_hop()  = IP_BROADCAST;
        ch->addr_type() = NS_AF_INET;
        ch->uid()       = packet_sequence;
        ih->saddr()     = id;
        ih->sport()     = RT_PORT;
        ih->daddr()     = group->multicast_address;
        ph->ma_count    = 0;

        if (am_i_mesh_member(group)) {
            message_cache.add(ih->saddr(), packet_sequence);
            routing_set_timer(new FlushTablesEvent(), FLUSH_INTERVAL);
            Scheduler::instance().schedule(target_, p,
                                        BROADCAST_JITTER * random.uniform(1.0));
        }
        else
            forward_fresh_data_packet(group, p);
}

void
PUMA::send_a_partition_confirmation_request(multicast_group* group) {
        nsaddr_t temp = group->core_id;
        group->core_id = TENTATIVE_PARTITION;
        send_a_generic_ma(group, PARTITION, temp);
        group->core_id = temp;
}

void
PUMA::send_a_single_ma(multicast_group* group) {
        send_a_generic_ma(group, NORMAL, INVALID_ADDRESS);
}

void
PUMA::send_a_generic_ma(multicast_group* group, int mode, nsaddr_t oldCore) {
		multicast_announcement ma;
        Packet* p = Packet::alloc(sizeof(ma));

        hdr_cmn   *ch = HDR_CMN(p);
        hdr_ip    *ih = HDR_IP(p);
        hdr_puma  *ph = HDR_PUMA(p);
        ma.multicast_address = group->multicast_address;
        ma.core_id           = group->core_id;
        ma.distance_to_core  = distance_to_core(group);
        ma.sequence          = group->last_sequence;
        ma.mesh_member       = group->mesh_member = am_i_mesh_member(group);
        if (mode == NORMAL)
            ma.next_hop      = get_next_hop(group);
        else
            ma.next_hop      = oldCore;

        ch->ptype()     = PT_PUMA;
        ch->direction() = hdr_cmn::DOWN;
        ch->size()      += IP_HDR_LEN + PUMA_HDR_LEN;
        ch->error()     = 0;
        ch->prev_hop_   = id;
        ch->next_hop()  = IP_BROADCAST;
        ch->addr_type() = NS_AF_INET;
        ih->saddr()     = id;
        ih->daddr()     = group->multicast_address;
        ih->sport()     = RT_PORT;
        ih->dport()     = RT_PORT;
        ih->ttl()       = IP_DEF_TTL;
        ph->ma_count    = 1;

        *((multicast_announcement*)((PacketData*)p->userdata())->data()) = ma;
        Scheduler::instance().schedule(target_, p,
                                       BROADCAST_JITTER * random.uniform(1.0));
}

void PUMA::handle_ma(multicast_group* group, multicast_announcement ma,
                     nsaddr_t source) {
        if (ma.core_id == INVALID_ADDRESS) {
            if (group != NULL && group->core_id != INVALID_ADDRESS) {
                send_a_single_ma(group);
            }
        }
        else if (ma.core_id == TENTATIVE_PARTITION) {
            if (distance_to_core(group) < ma.distance_to_core)
                handle_partition_confirmation_request(group, ma.next_hop,
                                                       ma.sequence);
        }
        else if (group->core_id == INVALID_ADDRESS ||
                 ma.core_id > group->core_id) {
                // Reconnection
            update_group(group, ma, source);
            send_a_single_ma(group);
        }
        else if (ma.core_id != group->core_id &&
            is_partition_flag_set(group)) {
            // Partition
            update_group(group, ma, source);
            send_a_single_ma(group);
        }
        else if (ma.core_id < group->core_id) {
            // Useless MA.
        }
        else {
            bool mesh_membership_after;
            if (ma.sequence  > group->last_sequence)
                // Fresher MA.
                handle_fresher_ma(group, ma, source);
            else if (ma.sequence < group->last_sequence) {
                group->connectivity_list->add(source, ma.distance_to_core + 1,
						              ma.sequence, ma.next_hop, ma.mesh_member);
            }
            else if ((ma.distance_to_core + 1) < distance_to_core(group)) {
                group->connectivity_list->add(source, ma.distance_to_core + 1,
                                      ma.sequence, ma.next_hop, ma.mesh_member);
                if (is_single_group_present())
                    send_a_delayed_ma(group);
            }
            else {
                group->connectivity_list->add(source, ma.distance_to_core + 1,
                                      ma.sequence, ma.next_hop, ma.mesh_member);
            }
            mesh_membership_after = am_i_mesh_member(group);
            if (group->mesh_member != mesh_membership_after &&
                is_single_group_present())
                send_a_delayed_ma(group);
        }
}

void
PUMA::send_a_delayed_ma(multicast_group* group) {
        if (!(group->delayed_ma_pending)) {
            group->delayed_ma_pending = true;
            routing_set_timer(new SendPendingMAEvent(group->multicast_address),
                                MULTICAST_ANNOUNCEMENT_DELAY);
        }
}

bool
PUMA::is_single_group_present() {
        return (multicast_group_list.size() == 1);
}

void
PUMA::handle_fresher_ma(multicast_group* group, multicast_announcement ma,
                       nsaddr_t source) {
        group->last_sequence = ma.sequence;
        group->last_ma_received = NOW;
        group->connectivity_list->add(source,
                ma.distance_to_core + 1, ma.sequence,
                ma.next_hop, ma.mesh_member);
        if (is_single_group_present() || group->partition_confirmation_wait)
            send_a_delayed_ma(group);
}

void
PUMA::handle_partition_confirmation_request(multicast_group* group,
                                          nsaddr_t current_core,
                                          u_int32_t sequence) {
        if (group->core_id == current_core &&
            group->last_sequence > sequence)
            send_a_single_ma(group);
        else if (!group->partition_confirmation_wait)
            fight_multiple_group_election_after_partition(group);
}

void
PUMA::send_next_bucket() {
        u_int32_t x = multicast_group_list.size();
        routing_set_timer(new SendNextBucketEvent(), MULTICAST_ANNOUNCEMENT);
        multicast_group* group = multicast_group_list.get_front();
        if (group != NULL && !is_single_group_present() && x > 0) {
            multicast_announcement bucket[x];
            Packet *p = Packet::alloc(sizeof(bucket));
            hdr_cmn     *ch = HDR_CMN(p);
            hdr_ip      *ih = HDR_IP(p);
            hdr_puma    *ph = HDR_PUMA(p);

            ch->ptype()     = PT_PUMA;
            ch->direction() = hdr_cmn::DOWN;
            ch->size()      += IP_HDR_LEN + PUMA_HDR_LEN;
            ch->error()     = 0;
            ch->prev_hop_   = id;
            ch->addr_type() = NS_AF_INET;
            ch->next_hop()  = IP_BROADCAST;

            ih->saddr()     = id;
            ih->daddr()     = group->multicast_address;
            ih->sport()     = RT_PORT;
            ih->dport()     = RT_PORT;
            ih->ttl()       = IP_DEF_TTL;

            ph->ma_count    = x;

            int i = 0;
            multicast_group* temp = group;
            while (temp != NULL) {
                if (is_partition_flag_set(temp) && temp->receiver)
	                if (!(temp->partition_confirmation_wait))
		                fight_multiple_group_election_after_partition(temp);
                bucket[i].multicast_address= temp->multicast_address;
                bucket[i].core_id          = temp->core_id;
                bucket[i].distance_to_core = distance_to_core(temp);
                bucket[i].sequence         = temp->last_sequence;
                bucket[i].next_hop         = get_next_hop(temp);
                bucket[i].mesh_member      = temp->mesh_member =
                                             am_i_mesh_member(temp);
                i++;
                temp = temp->next;
            }
            memcpy((void*)(((PacketData*)p->userdata())->data()),(void*)&bucket,
                 sizeof(bucket));
            Scheduler::instance().schedule(target_, p,
                                       BROADCAST_JITTER * random.uniform(1.0));
        }
        else if (is_single_group_present() && is_partition_flag_set(group) &&
                group->receiver)
             become_core_after_partition(group);
}

void
PUMA::handle_bucket(Packet* p) {
        hdr_ip    *ih = HDR_IP(p);
        hdr_puma *ph = HDR_PUMA(p);
        if (ih->saddr() == id) {
            Packet::free(p);
            return;
        }
        record_ack(ih->saddr());
        multicast_announcement bucket[ph->ma_count];
        memcpy((void*)&bucket,(void*)((PacketData*)p->userdata())->data(),
                sizeof(multicast_announcement) * ph->ma_count);
        nsaddr_t group_address;
        for (u_int32_t i = 0; i < ph->ma_count; i++) {
            group_address = bucket[i].multicast_address;
            multicast_group* group = multicast_group_list.find(group_address);
            if (group == NULL) {
                multicast_group_list.add(group_address, INVALID_ADDRESS);
                group = multicast_group_list.find(group_address);
            }
            handle_ma(group, bucket[i], ih->saddr());
        }
        Packet::free(p);
}

void
PUMA::send_next_ma(multicast_group* group) {
        if (group != NULL && group->core_id == id) {
            book_keeping_before_ma(group);
            if (is_single_group_present()) {
                send_a_single_ma(group);
            }
        }
}

void
PUMA::book_keeping_before_ma(multicast_group* group) {
        routing_set_timer(new SendNextMAEvent(group->multicast_address),
                          MULTICAST_ANNOUNCEMENT);
        group->last_ma_originated = NOW;
        (group->last_sequence)++;
}

void
PUMA::record_ack(nsaddr_t source) {
        (ack_table.get_entries())++;
        if (ack_table.get_entries() > NUMBER_OF_ACK_TABLE_ENTRIES)
            ack_table.get_entries() = NUMBER_OF_ACK_TABLE_ENTRIES;
        ack_table.ack_table_data[ack_table.get_empty_slot()].time_heard = NOW;
        ack_table.ack_table_data[ack_table.get_empty_slot()].heard_from =source;
        (ack_table.get_empty_slot())++;
        ack_table.get_empty_slot() = ack_table.get_empty_slot() %
		                                            NUMBER_OF_ACK_TABLE_ENTRIES;
}

nsaddr_t
PUMA::get_next_hop(multicast_group* group) {
        if ((group == NULL) || (group->connectivity_list->get_front() == NULL))
            return INVALID_ADDRESS;
        else if ((group->core_id == id))
            return INVALID_ADDRESS;
        else {
            connectivity_item* temp = group->connectivity_list->get_front();
            u_int32_t i;
            for (i=1; i < NUMBER_OF_PARENTS; i++) {
                if (temp->next == NULL)
                    break;
                if (temp->next->sequence !=
                    group->connectivity_list->get_front()->sequence ||

                    temp->next->distance_to_core !=
                    group->connectivity_list->get_front()->distance_to_core)
                    break;
                temp = temp->next;
            }
            return temp->next_hop;
        }
}

u_int16_t
PUMA::distance_to_core(multicast_group* group) {
        if ((group != NULL) && (group->core_id == id))
            return 0;
        if ((group == NULL) || (group->connectivity_list->get_front() == NULL))
            return INVALID_DISTANCE;
        return group->connectivity_list->get_front()->distance_to_core;
}
