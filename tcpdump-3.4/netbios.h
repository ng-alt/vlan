/*
 * NETBIOS protocol formats
 *
 * @(#) $Header: /home/cvs/vlan/vlan/tcpdump-3.4/netbios.h,v 1.1 1999/10/17 20:01:14 greear Exp $
 */

struct p8022Hdr {
    u_char	dsap;
    u_char	ssap;
    u_char	flags;
};

#define	p8022Size	3		/* min 802.2 header size */

#define UI		0x03		/* 802.2 flags */

