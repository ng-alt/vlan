/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/cvs/vlan/vlan/tcpdump-3.4/print-ether.c,v 1.2 1999/10/20 04:04:43 greear Exp $ (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>

#include <stdio.h>
#include <pcap.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"

const u_char *packetp;
const u_char *snapend;

static inline void
ether_print(register const u_char *bp, u_int length)
{
	register const struct ether_header *ep;
        register const struct vlan_ether_header *vep;

	ep = (const struct ether_header *)bp;
        vep = (const struct vlan_ether_header*)bp;
        
	if (qflag)
		(void)printf("%s %s %d: ",
			     etheraddr_string(ESRC(ep)),
			     etheraddr_string(EDST(ep)),
			     length);
	else {
           /* Header is different if it's a VLAN packet */
           /* put some things in Host Byte Order */
           unsigned short tci = ntohs(vep->h_vlan_TCI);
           if (ntohs(ep->ether_type) == ETHERTYPE_VLAN) {
              (void)printf("%s %s VLAN Pri: %hx CFI: %d VID: %hx %s %d  ",
                           etheraddr_string(ESRC(ep)),
                           etheraddr_string(EDST(ep)),
                           (tci >> 5),
                           (tci & (1 << 4)), /* CFI */
                           (tci & 0xFFF),
                           etherproto_string(ntohs(vep->h_vlan_encapsulated_proto)),
                           length);
           }
           else {
              (void)printf("%s %s %s %d: ",
                           etheraddr_string(ESRC(ep)),
                           etheraddr_string(EDST(ep)),
                           etherproto_string(ntohs(ep->ether_type)),
                           length);
           }
        }
}/* ether_print */


/*
 * This is the top level routine of the printer.  'p' is the points
 * to the ether header of the packet, 'tvp' is the timestamp,
 * 'length' is the length of the packet off the wire, and 'caplen'
 * is the number of bytes actually captured.
 */
void
ether_if_print(u_char *user, const struct pcap_pkthdr *h, const u_char *p)
{
	u_int caplen = h->caplen;
	u_int length = h->len;
	struct ether_header *ep;
        struct vlan_ether_header *vep;
        unsigned short header_len;
        unsigned char is_vlan = 0;
	u_short ether_type;
	extern u_short extracted_ethertype;

	ts_print(&h->ts);

	if (caplen < sizeof(struct ether_header)) {
		printf("[|ether]");
		goto out;
	}

	if (eflag)
		ether_print(p, length);

	/*
	 * Some printers want to get back at the ethernet addresses,
	 * and/or check that they're not walking off the end of the packet.
	 * Rather than pass them all the way down, we set these globals.
	 */
	packetp = p;
	snapend = p + caplen;

	ep = (struct ether_header *)p;

/* VLAN CHANGES HERE */
        vep = (struct vlan_ether_header*)p;
        
        if (ntohs(ep->ether_type) == ETHERTYPE_VLAN) {
           is_vlan = 1; /* TRUE */
           header_len = sizeof(struct vlan_ether_header);
           ether_type = ntohs(vep->h_vlan_encapsulated_proto);
        }
        else {
           is_vlan = 0; /* FALSE */
           header_len = sizeof(struct ether_header);
           ether_type = ntohs(ep->ether_type);
        }
        
	length -= header_len;
	caplen -= header_len;
        
	p += header_len; /* walk past ether header */

/* END OF VLAN CHANGES */

        
	/*
	 * Is it (gag) an 802.3 encapsulation?
	 */
	extracted_ethertype = 0;
	if (ether_type <= ETHERMTU) {
		/* Try to print the LLC-layer header & higher layers */
		if (llc_print(p, length, caplen, ESRC(ep), EDST(ep)) == 0) {
			/* ether_type not known, print raw packet */
			if (!eflag)
				ether_print((u_char *)ep, length);
			if (extracted_ethertype) {
				printf("(LLC %s) ",
			       etherproto_string(htons(extracted_ethertype)));
			}
			if (!xflag && !qflag)
				default_print(p, caplen);
		}
	} else if (ether_encap_print(ether_type, p, length, caplen) == 0) {
		/* ether_type not known, print raw packet */
		if (!eflag)
			ether_print((u_char *)ep, length + sizeof(*ep));
		if (!xflag && !qflag)
			default_print(p, caplen);
	}
	if (xflag)
		default_print(p, caplen);

        /* Print out the entire packet, including the ethernet
         * header (and anything at the end of the payload??)
         * I will be using the output of this in a seperate program
         * that will be able to manually edit such packets and poke
         * it back on an ethernet device (on linux at least)
         * --BLG
         */
        if (raw_eth_flag) {
           putchar('\n');
           default_print(packetp, snapend - packetp);
        }
  out:
	putchar('\n');
}

/*
 * Prints the packet encapsulated in an Ethernet data segment
 * (or an equivalent encapsulation), given the Ethernet type code.
 *
 * Returns non-zero if it can do so, zero if the ethertype is unknown.
 *
 * Stuffs the ether type into a global for the benefit of lower layers
 * that might want to know what it is.
 */

u_short	extracted_ethertype;

int
ether_encap_print(u_short ethertype, const u_char *p,
    u_int length, u_int caplen)
{
	extracted_ethertype = ethertype;

	switch (ethertype) {

	case ETHERTYPE_IP:
		ip_print(p, length);
		return (1);

	case ETHERTYPE_ARP:
	case ETHERTYPE_REVARP:
		arp_print(p, length, caplen);
		return (1);

	case ETHERTYPE_DN:
		decnet_print(p, length, caplen);
		return (1);

	case ETHERTYPE_ATALK:
		if (vflag)
			fputs("et1 ", stdout);
		atalk_print(p, length);
		return (1);

	case ETHERTYPE_AARP:
		aarp_print(p, length);
		return (1);

	case ETHERTYPE_LAT:
	case ETHERTYPE_SCA:
	case ETHERTYPE_MOPRC:
	case ETHERTYPE_MOPDL:
		/* default_print for now */
	default:
		return (0);
	}
}
