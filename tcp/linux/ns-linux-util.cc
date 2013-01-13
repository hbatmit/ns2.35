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
 * Module: linux/ns-linux-util.cc
 *	This is the utilities for linkages between NS-2 source codes (in C++) and Linux source codes (in C)
 *
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */
#include <string.h>
#include "ns-linux-util.h"
__u32 tcp_time_stamp=0;
s64 ktime_get_real=0;

struct list_head ns_tcp_cong_list={&ns_tcp_cong_list, &ns_tcp_cong_list};
struct list_head *last_added = &ns_tcp_cong_list;

/* A list of parameters for different cc 
struct cc_param_list {
	const char* name;
	const char* type;
	const char* description;
	struct cc_param_list* next;
};
struct cc_list {
	const char* proto;
	struct cc_param_list* head;
	struct cc_list* next;
}
*/
struct cc_list* cc_list_head = NULL;

unsigned char cc_list_changed = 0;


