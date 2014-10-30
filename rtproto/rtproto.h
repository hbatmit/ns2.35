#ifndef __rtproto_h__
#define __rtproto_h__

#include <agent.h>
#include <packet.h>
#include <sys/types.h>

class rtAgent : public Agent {
public:
        rtAgent(nsaddr_t index, packet_t pt) : Agent(pt), ipaddr_(index) { }

	// ============================================================
	// Routing API (used by IMEP layer)

	virtual void rtNotifyLinkUP(nsaddr_t index) = 0;
	virtual void rtNotifyLinkDN(nsaddr_t index) = 0;
	virtual void rtNotifyLinkStatus(nsaddr_t index, u_int32_t status) = 0;

	virtual void rtRoutePacket(Packet *p) = 0;

	// ============================================================

protected:
	virtual inline int initialized() = 0;

	nsaddr_t& ipaddr() { return ipaddr_; }

private:
	nsaddr_t	ipaddr_;	// IP address of this node
};

#endif /* __rtproto_h__ */

