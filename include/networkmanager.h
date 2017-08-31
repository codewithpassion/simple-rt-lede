#include <stdint.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <netdb.h>

#ifndef _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

ssize_t wrapIPpackageInEthernet(uint8_t *acc_buf, size_t size, uint8_t *sendbuf);

uint8_t *allocate_ustrmem(int len);
char * allocate_strmem(int len);

void handleArpRequest(uint8_t *acc_buf, size_t size, int tunnelDevice);

// Define an struct for ARP header
typedef struct _arp_hdr arp_hdr;
struct _arp_hdr
{
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];
};

#define ETH_HDRLEN 14    // Ethernet header length
#define IP4_HDRLEN 20    // IPv4 header length
#define ARP_HDRLEN 28    // ARP header length
#define ARPOP_REQUEST 1  // Taken from <linux/if_arp.
#define ARPOP_RESPONSE 2 // Taken from <linux/if_arp.

#define IP_MAXPACKET 65535

#endif