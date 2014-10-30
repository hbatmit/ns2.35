/* 
 * TCP-Linux module for NS2 
 *
 * May 2006
 *
 * Author: Xiaoliang (David) Wei  (DavidWei@acm.org)
 *
 * NetLab, the California Institute of Technology 
 * http://netlab.caltech.edu
 *
 * Module: linux/ns-linux-util.h
 *      This is the header file for linkages between NS-2 source codes (in C++) and Linux source codes (in C)
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */

#ifndef NS_LINUX_UTIL_H
#define NS_LINUX_UTIL_H
#include <stdlib.h>
#include <stdint.h>
#include "ns-linux-param.h"

extern struct tcp_congestion_ops tcp_reno;

extern unsigned long tcp_time_stamp;
extern long long ktime_get_real;

#define JIFFY_RATIO 1000
#define US_RATIO 1000000
#define MS_RATIO 1000

#define jiffies_to_usecs(x) ((US_RATIO/JIFFY_RATIO)*(x))
#define msecs_to_jiffies(x) ((JIFFY_RATIO/MS_RATIO)*(x))

extern void tcp_cong_avoid_register(void);

#define __u64 unsigned long long
#define __u32 unsigned long
#define __u16 unsigned int
#define __u8 unsigned char

#define u64 __u64
#define u32 __u32
#define u16 __u16
#define u8 __u8

#define s32 long
#define s64 long long

#define ktime_t s64
extern ktime_t net_invalid_timestamp();
extern int ktime_equal(const ktime_t cmp1, const ktime_t cmp2);
extern s64 ktime_to_us(const ktime_t kt);
extern ktime_t net_timedelta(ktime_t t);

#define inet_csk(sk) (sk)
#define tcp_sk(sk) (sk)
#define inet_csk_ca(sk) (void*)((sk)->icsk_ca_priv)


//from kernel.h
#define min_t(type,x,y) \
	(((type)(x)) < ((type)(y)) ? ((type)(x)): ((type)(y)))

#define max_t(type,x,y) \
	(((type)(x)) > ((type)(y)) ? ((type)(x)): ((type)(y)))


//#define max(x,y) ((x>y)? x:y)

/* Events passed to congestion control interface */
enum tcp_ca_event {
	CA_EVENT_TX_START,	/* first transmit when no packets in flight */
	CA_EVENT_CWND_RESTART,	/* congestion window restart */
	CA_EVENT_COMPLETE_CWR,	/* end of congestion recovery */
	CA_EVENT_FRTO,		/* fast recovery timeout */
	CA_EVENT_LOSS,		/* loss timeout */
	CA_EVENT_FAST_ACK,	/* in sequence ack */
	CA_EVENT_SLOW_ACK,	/* other ack */
};

#define sock tcp_sock
#define inet_sock tcp_sock
#define inet_connection_sock tcp_sock
struct sk_buff;
struct sock;


/* A list of parameters for different cc */
struct cc_param_list {
	const char* name;
	const char* type;
	const char* description;
	const void* ptr;
	struct cc_param_list* next;
};

struct cc_list {
	const char* proto;
	struct cc_param_list* param_head;
	struct cc_list* next;
};
extern struct cc_list* cc_list_head;

/* List Type purely for tcp_cong_list*/
struct list_head {
	struct list_head *prev;
	struct list_head *next;
	const char* file_name;
};

extern unsigned char cc_list_changed;
extern struct list_head ns_tcp_cong_list;
extern struct list_head *last_added;
#define list_for_each_entry_rcu(pos, head, member) \
	for (pos=(struct tcp_congestion_ops*)ns_tcp_cong_list.next; pos!=(struct tcp_congestion_ops*)(&ns_tcp_cong_list); pos=(struct tcp_congestion_ops*)pos->member.next) 

#define list_add_rcu(a,b) {\
	(a)->next=ns_tcp_cong_list.next;\
	(a)->prev=(&ns_tcp_cong_list);\
	ns_tcp_cong_list.next->prev=(a);\
	ns_tcp_cong_list.next=(a);\
	last_added = (a);\
	cc_list_changed = 1;\
}

#define list_del_rcu(a) {(a)->prev->next=(a)->next; (a)->next->prev=(a)->prev; cc_list_changed = 1; }
#define list_move(a,b) {list_del_rcu(a); list_add_rcu(a,b);}
#define list_entry(a,b,c) ((b*)ns_tcp_cong_list.next)
#define list_add_tail_rcu(a,b) {\
	(a)->prev=ns_tcp_cong_list.prev;\
	(a)->next=(&ns_tcp_cong_list);\
	ns_tcp_cong_list.prev->next=(a);\
	ns_tcp_cong_list.prev=(a);\
	last_added = (a);\
	cc_list_changed = 1;\
}

/*
 * Interface for adding new TCP congestion control handlers
 */
#define TCP_CA_NAME_MAX	16
#define TCP_CONG_NON_RESTRICTED 0x1
#define TCP_CONG_RTT_STAMP      0x2
struct tcp_congestion_ops {
	struct list_head	list;

	unsigned long flags;
	int non_restricted;

	/* initialize private data (optional) */
	void (*init)(struct sock *sk);
	/* cleanup private data  (optional) */
	void (*release)(struct sock *sk);

	/* return slow start threshold (required) */
	u32 (*ssthresh)(struct sock *sk);
	/* lower bound for congestion window (optional) */
	u32 (*min_cwnd)(const struct sock *sk);
	/* do new cwnd calculation (required) */
	void (*cong_avoid)(struct sock *sk, u32 ack,
			   u32 rtt, u32 in_flight, int good_ack);
	/* round trip time sample per acked packet (optional) */
	void (*rtt_sample)(struct sock *sk, u32 usrtt);
	/* call before changing ca_state (optional) */
	void (*set_state)(struct sock *sk, u8 new_state);
	/* call when cwnd event occurs (optional) */
	void (*cwnd_event)(struct sock *sk, enum tcp_ca_event ev);
	/* new value of cwnd after loss (optional) */
	u32  (*undo_cwnd)(struct sock *sk);
	/* hook for packet ack accounting (optional) */
	void (*pkts_acked)(struct sock *sk, u32 num_acked, ktime_t last);
	/* get info for inet_diag (optional) */
	void (*get_info)(struct sock *sk, u32 ext, struct sk_buff *skb);

	char 		name[TCP_CA_NAME_MAX];
	struct module 	*owner;
};

struct tcp_options_received {
/*	PAWS/RTTM data	*/
//	long	ts_recent_stamp;/* Time we stored ts_recent (for aging) */
//	__u32	ts_recent;	/* Time stamp to echo next		*/
	__u32	rcv_tsval;	/* Time stamp value             	*/
	__u32	rcv_tsecr;	/* Time stamp echo reply        	*/
	__u16 	saw_tstamp : 1,	/* Saw TIMESTAMP on last packet		*/
		dump_xxx: 15;
//		tstamp_ok : 1,	/* TIMESTAMP seen on SYN packet		*/
//		dsack : 1,	/* D-SACK is scheduled			*/
//		wscale_ok : 1,	/* Wscale seen on SYN packet		*/
//		sack_ok : 4,	/* SACK seen on SYN packet		*/
//		snd_wscale : 4,	/* Window scaling received from sender	*/
//		rcv_wscale : 4;	/* Window scaling to send to receiver	*/
/*	SACKs data	*/
//	__u8	eff_sacks;	/* Size of SACK array to send with next packet */
//	__u8	num_sacks;	/* Number of SACK blocks		*/
///	__u16	user_mss;  	/* mss requested by user in ioctl */
//	__u16	mss_clamp;	/* Maximal mss, negotiated at connection setup */
};

struct tcp_sock {
/* inet_connection_sock has to be the first member of tcp_sock */
//	struct inet_connection_sock	inet_conn;
	int	sk_state;
// 	int	tcp_header_len;	/* Bytes of tcp header to send		*/

/*
 *	Header prediction flags
 *	0x5?10 << 16 + snd_wnd in net byte order
 */
//	__u32	pred_flags;

/*
 *	RFC793 variables by their proper names. This means you can
 *	read the code and the spec side by side (and laugh ...)
 *	See RFC793 and RFC1122. The RFC writes these in capitals.
 */
// 	__u32	rcv_nxt;	/* What we want to receive next 	*/
 	__u32	snd_nxt;	/* Next sequence we send		*/

 	__u32	snd_una;	/* First byte we want an ack for	*/
// 	__u32	snd_sml;	/* Last byte of the most recently transmitted small packet */
//	__u32	rcv_tstamp;	/* timestamp of last received ACK (for keepalives) */
//	__u32	lsndtime;	/* timestamp of last sent data packet (for restart window) */

	/* Data for direct copy to user */
//	struct {
//		struct sk_buff_head	prequeue;
//		struct task_struct	*task;
//		struct iovec		*iov;
//		int			memory;
//		int			len;
//	} ucopy;

//	__u32	snd_wl1;	/* Sequence for window update		*/
//	__u32	snd_wnd;	/* The window we expect to receive	*/
//	__u32	max_window;	/* Maximal window ever seen from peer	*/
//	__u32	pmtu_cookie;	/* Last pmtu seen by socket		*/
	__u32	mss_cache;	/* Cached effective mss, not including SACKS */
//	__u16	xmit_size_goal;	/* Goal for segmenting output packets	*/
//	__u16	ext_header_len;	/* Network protocol overhead (IP/IPv6 options) */
//
//	__u32	window_clamp;	/* Maximal window to advertise		*/
//	__u32	rcv_ssthresh;	/* Current window clamp			*/
//
//	__u32	frto_highmark;	/* snd_nxt when RTO occurred */
//	__u8	reordering;	/* Packet reordering metric.		*/
//	__u8	frto_counter;	/* Number of new acks after RTO */
//	__u8	nonagle;	/* Disable Nagle algorithm?             */
//	__u8	keepalive_probes; /* num of allowed keep alive probes	*/

/* RTT measurement */
	__u32	srtt;		/* smoothed round trip time << 3	*/
//	__u32	mdev;		/* medium deviation			*/
//	__u32	mdev_max;	/* maximal mdev for the last rtt period	*/
//	__u32	rttvar;		/* smoothed mdev_max			*/
//	__u32	rtt_seq;	/* sequence number to update rttvar	*/
//
//	__u32	packets_out;	/* Packets which are "in flight"	*/
//	__u32	left_out;	/* Packets which leaved network	*/
//	__u32	retrans_out;	/* Retransmitted packets out		*/
/*
 *      Options received (usually on last packet, some only on SYN packets).
 */
	struct tcp_options_received rx_opt;

/*
 *	Slow start and congestion control (see also Nagle, and Karn & Partridge)
 */
 	__u32	snd_ssthresh;	/* Slow start size threshold		*/
	__u32	snd_cwnd;	/* Sending congestion window		*/
 	__u16	snd_cwnd_cnt;	/* Linear increase counter		*/
	__u16	snd_cwnd_clamp; /* Do not allow snd_cwnd to grow above this */
//	__u32	snd_cwnd_used;
	__u32	snd_cwnd_stamp;
	__u32	bytes_acked;
//
//	struct sk_buff_head	out_of_order_queue; /* Out of order segments go here */
//
//	struct tcp_func		*af_specific;	/* Operations which are AF_INET{4,6} specific	*/
//
 //	__u32	rcv_wnd;	/* Current receiver window		*/
//	__u32	rcv_wup;	/* rcv_nxt on last window update sent	*/
//	__u32	write_seq;	/* Tail(+1) of data held in tcp send buffer */
//	__u32	pushed_seq;	/* Last pushed seq, required to talk to windows */
//	__u32	copied_seq;	/* Head of yet unread data		*/
//
/*	SACKs data	*/
//	struct tcp_sack_block duplicate_sack[1]; /* D-SACK block */
//	struct tcp_sack_block selective_acks[4]; /* The SACKS themselves*/

//	__u16	advmss;		/* Advertised MSS			*/
//	__u16	prior_ssthresh; /* ssthresh saved at recovery start	*/
//	__u32	lost_out;	/* Lost packets			*/
//	__u32	sacked_out;	/* SACK'd packets			*/
//	__u32	fackets_out;	/* FACK'd packets			*/
//	__u32	high_seq;	/* snd_nxt at onset of congestion	*/
//
//	__u32	retrans_stamp;	/* Timestamp of the last retransmit,
//				 * also used in SYN-SENT to remember stamp of
//				 * the first SYN. */
//	__u32	undo_marker;	/* tracking retrans started here. */
//	int	undo_retrans;	/* number of undoable retransmissions. */
//	__u32	urg_seq;	/* Seq of received urgent pointer */
//	__u16	urg_data;	/* Saved octet of OOB data and control flags */
//	__u8	urg_mode;	/* In urgent mode		*/
//	__u8	ecn_flags;	/* ECN status bits.			*/
//	__u32	snd_up;		/* Urgent pointer		*/
//
//	__u32	total_retrans;	/* Total retransmits for entire connection */
//
//	unsigned int		keepalive_time;	  /* time before keep alive takes place */
//	unsigned int		keepalive_intvl;  /* time interval between keep alive probes */
//	int			linger2;
//
//	unsigned long last_synq_overflow; 
//
/* Receiver side RTT estimation */
//	struct {
//		__u32	rtt;
//		__u32	seq;
//		__u32	time;
//	} rcv_rtt_est;

/* Receiver queue space */
//	struct {
//		int	space;
//		__u32	seq;
//		__u32	time;
//	} rcvq_space;
	struct tcp_congestion_ops *icsk_ca_ops;
	__u8			  icsk_ca_state;
	u32			  icsk_ca_priv[16];
#define ICSK_CA_PRIV_SIZE	(16 * sizeof(u32))
};

struct sk_buff {

};

extern struct tcp_congestion_ops tcp_init_congestion_ops;


enum tcp_ca_state
{
	TCP_CA_Open = 0,
#define TCPF_CA_Open	(1<<TCP_CA_Open)
	TCP_CA_Disorder = 1,
#define TCPF_CA_Disorder (1<<TCP_CA_Disorder)
	TCP_CA_CWR = 2,
#define TCPF_CA_CWR	(1<<TCP_CA_CWR)
	TCP_CA_Recovery = 3,
#define TCPF_CA_Recovery (1<<TCP_CA_Recovery)
	TCP_CA_Loss = 4
#define TCPF_CA_Loss	(1<<TCP_CA_Loss)
};

#define FLAG_ECE 1
#define FLAG_DATA_SACKED 2
#define FLAG_DATA_ACKED 4
#define FLAG_DATA_LOST 8
#define FLAG_CA_ALERT           (FLAG_DATA_SACKED|FLAG_ECE)
#define FLAG_NOT_DUP		(FLAG_DATA_ACKED)

#define FLAG_UNSURE_TSTAMP 16

#define CONFIG_DEFAULT_TCP_CONG "reno"
#define TCP_CLOSE 0

#endif
