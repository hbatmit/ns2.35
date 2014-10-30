#ifndef _NET_ETHERNET_H_
#define _NET_ETHERNET_H_

#ifndef ETHER_ADDR_LEN
#define	ETHER_ADDR_LEN		6
#endif

#ifndef ETHER_TYPE_LEN
#define	ETHER_TYPE_LEN		2
#endif

#define	ETHER_CRC_LEN		4

#ifndef ETHER_HDR_LEN
#define	ETHER_HDR_LEN		(ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)
#endif

#ifndef ETHER_MIN_LEN
#define	ETHER_MIN_LEN		64
#endif

#ifndef ETHER_MAX_LEN
#define	ETHER_MAX_LEN		1518
#endif

#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

/*
 * Structure of a 10Mb/s Ethernet header.
 */
#ifndef HAVE_ETHER_HEADER_STRUCT
struct	ether_header {
	u_char	ether_dhost[ETHER_ADDR_LEN];
	u_char	ether_shost[ETHER_ADDR_LEN];
	u_short	ether_type;
};

#endif
/*
 * Structure of a 48-bit Ethernet address.
 */
#ifndef HAVE_ETHER_ADDRESS_STRUCT
struct	ether_addr {
	u_char octet[ETHER_ADDR_LEN];
};
#endif

#endif

#ifndef _NS_ETHERNET_H_
#define _NS_ETHERNET_H_


class Ethernet {
public: 
        static void ether_print(const u_char *);
        static char* etheraddr_string(const u_char*);
	static u_char* nametoaddr(const char *devname);

private:
        static char hex[];
};   

#endif
