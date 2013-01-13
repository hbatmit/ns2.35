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
 * Module: linux/ns-linux-param.c
 *	This is the list of linux parameters to be tuned.
 *
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */
#define NS_PROTOCOL "tcp_linux.c"
#include "ns-linux-c.h"
unsigned char sysctl_tcp_abc=1;
module_param(sysctl_tcp_abc, int, 0644);
MODULE_PARM_DESC(sysctl_tcp_abc, "whether we needs ABC or not");

unsigned char tcp_max_burst=3;
module_param(tcp_max_burst, int, 0644);
MODULE_PARM_DESC(tcp_max_burst, "the maximum burst size of a TCP");

unsigned char debug_level=1;
module_param(debug_level, int, 0644);
MODULE_PARM_DESC(debug_level, "The debug level: 0: INFO; <=1: NOTICE; <=2: ERROR");


