/*
 * SimpleRT: Reverse tethering utility for Android
 * Copyright (C) 2016 Konstantin Menyaev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NETWORH_H_
#define _NETWORH_H_

#include <stdint.h>
#include <stdbool.h>

#include "accessory.h"

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define IP_MAXPACKET 65535
#define BUF_SIZ 1024

bool start_network(void);
void stop_network(void);

ssize_t send_network_packet(const uint8_t *data, size_t size);

accessory_id_t get_acc_id_from_packet(const uint8_t *data,
        size_t size, bool dst_addr);

char *fill_serial_param(char *buf, size_t size,
        accessory_id_t acc_id);

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

#define ETH_HDRLEN 14   // Ethernet header length
#define IP4_HDRLEN 20   // IPv4 header length
#define ARP_HDRLEN 28   // ARP header length
#define ARPOP_REQUEST 1 // Taken from <linux/if_arp.
#define ARPOP_RESPONSE 2 // Taken from <linux/if_arp.

#endif