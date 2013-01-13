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
 * Module: linux/ns-linux-c.c
 *      This is the utilities of shortcuts for Linux source codes (in C) 
 *      We shortcut most of the Linux system calls which are not related to congestion control.
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */
#ifndef NS_LINUX_C_C
#define NS_LINUX_C_C

#include "ns-linux-util.h"
#include "ns-linux-c.h"
int fls(int x)
{
        int r = 32;

        if (!x)
                return 0;
        if (!(x & 0xffff0000u)) {
                x <<= 16;
                r -= 16;
        }
        if (!(x & 0xff000000u)) {
                x <<= 8;
                r -= 8;
        }
        if (!(x & 0xf0000000u)) {
                x <<= 4;
                r -= 4;
        }
        if (!(x & 0xc0000000u)) {
                x <<= 2;
                r -= 2;
        }
        if (!(x & 0x80000000u)) {
                x <<= 1;
                r -= 1;
        }
        return r;
}

int fls64(__u64 x)
{
        __u32 h = x >> 32;
        if (h)
                return fls(h) + 32;
        return fls(x);
}

// set the file name of last_added file
void set_linux_file_name(const char* name) {
	if (last_added != &ns_tcp_cong_list) {
		last_added->file_name = name;
	};
};

#include <stdio.h>

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


struct cc_list* find_cc_by_proto(const char* proto) {
	struct cc_list* p = cc_list_head;
	while (p!=NULL && (strcmp(p->proto, proto)!=0)) p=p->next;
	if (p==NULL) {
		p = (struct cc_list*) malloc(sizeof (struct cc_list));
		p->proto = proto;
		p->param_head = NULL;
		p->next = cc_list_head;
		cc_list_head = p;
	};
	return p;
};
struct cc_param_list* find_param_by_proto_name(const char* proto, const char* name) {
	struct cc_list* p = find_cc_by_proto(proto);
	if (!p) return NULL;
	struct cc_param_list* q = p->param_head;
	while (q!=NULL && (strcmp(q->name, name)!=0)) q=q->next;
	if (q==NULL) {
		q = (struct cc_param_list*) malloc(sizeof (struct cc_param_list));
		q->name = name;
		q->type = NULL;
		q->description = NULL;
		q->ptr = NULL;
		q->next = p->param_head;
		p->param_head = q;
	};
	return q;
};
void record_linux_param(const char* proto, const char* name, const char* type, void* ptr) {
	struct cc_param_list* p = find_param_by_proto_name(proto, name);
	if (p) {
		p->type = type;
		p->ptr = ptr;
		//printf("%s %s %s %p\n", proto, name, type, ptr);
	} else {
		printf("failed to register a parameter: %s %s %s %p\n", proto, name, type, ptr);
	};
};
void record_linux_param_description(const char* proto, const char* name, const char* exp) {
	struct cc_param_list* p = find_param_by_proto_name(proto, name);
	if (p) {
		p->description = exp;
	} else {
		printf("failed to register a description for a parameter: %s %s %s\n", proto, name, exp);
	};
};


/* 64bit divisor, dividend and result. dynamic precision */
uint64_t div64_64(uint64_t dividend, uint64_t divisor)
{
	uint32_t high, d;

	high = divisor >> 32;
	if (high) {
		unsigned int shift = fls(high);

		d = divisor >> shift;
		dividend >>= shift;
	} else
		d = divisor;

	do_div(dividend, d);

	return dividend;
}

/* About ktime_t */
ktime_t net_invalid_timestamp() {
	return 0;
};

int ktime_equal(const ktime_t cmp1, const ktime_t cmp2)
{
	return cmp1 == cmp2;
}

s64 ktime_to_us(const ktime_t kt)
{
	return kt/1000;
}

ktime_t net_timedelta(ktime_t t)
{
	return ktime_get_real - t;
}


#endif
