/* This is a very naive Reno implementation, shown as an example on how to develop a new congestion control algorithm with TCP-Linux.
 *
 * See a mini-tutorial about TCP-Linux at: http://netlab.caltech.edu/projects/ns2tcplinux/
 *
 */
#define NS_PROTOCOL "tcp_naive_reno.c"

#include "ns-linux-c.h"
#include "ns-linux-util.h"

static int alpha = 1;
static int beta = 2;
module_param(alpha, int, 0644);
MODULE_PARM_DESC(alpha, "AI increment size of window (in unit of pkt/round trip time)");
module_param(beta, int, 0644);
MODULE_PARM_DESC(beta, "MD decrement portion of window: every loss the window is reduced by a proportion of 1/beta");

/* opencwnd */
void tcp_naive_reno_cong_avoid(struct tcp_sock *tp, u32 ack, u32 rtt, u32 in_flight, int flag)
{
	ack = ack;
	rtt = rtt;
	in_flight = in_flight;
	flag = flag;

	if (tp->snd_cwnd < tp->snd_ssthresh) {
		tp->snd_cwnd++;
	} else {
		if (tp->snd_cwnd_cnt >= tp->snd_cwnd) {
			tp->snd_cwnd += alpha;
			tp->snd_cwnd_cnt = 0;
			if (tp->snd_cwnd > tp->snd_cwnd_clamp)
				tp->snd_cwnd = tp->snd_cwnd_clamp;
		} else {
			tp->snd_cwnd_cnt++;
		}
	}
}

/* ssthreshold should be half of the congestion window after a loss */
u32 tcp_naive_reno_ssthresh(struct tcp_sock *tp)
{
	int reduction = tp->snd_cwnd / beta;
        return max(tp->snd_cwnd - reduction, 2U);
}


/* congestion window should be equal to the slow start threshold (after slow start threshold set to half of cwnd before loss). */
u32 tcp_naive_reno_min_cwnd(const struct tcp_sock *tp)
{
        return tp->snd_ssthresh;
}

static struct tcp_congestion_ops tcp_naive_reno = {
        .name           = "naive_reno",
        .ssthresh       = tcp_naive_reno_ssthresh,
        .cong_avoid     = tcp_naive_reno_cong_avoid,
        .min_cwnd       = tcp_naive_reno_min_cwnd
};

int tcp_naive_reno_register(void)
{
	tcp_register_congestion_control(&tcp_naive_reno);
	return 0;
}
module_init(tcp_naive_reno_register);


