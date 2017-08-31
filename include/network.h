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


#define SIMPLERT_NETWORK_ADDRESS_BUILDER(a, b, c, d) ( \
    (uint32_t)((a) << 24) |                            \
    (uint32_t)((b) << 16) |                            \
    (uint32_t)((c) << 8) |                             \
    (uint32_t)((d) << 0))

#define SIMPLERT_NETWORK_ADDRESS \
        SIMPLERT_NETWORK_ADDRESS_BUILDER(10, 1, 1, 0)

#define NETWORK_ADDRESS(addr) \
        ((addr)&0xffffff00)

#define ACC_ID_FROM_ADDR(addr) \
        ((addr)&0xff)


bool start_network(void);
void stop_network(void);

ssize_t send_network_packet(const uint8_t *data, size_t size);

accessory_id_t get_acc_id_from_packet_offset(const uint8_t *data,
        size_t size, int offset, bool checkV4);
accessory_id_t get_acc_id_from_packet(const uint8_t *data,
        size_t size, bool dst_addr);

accessory_id_t get_acc_id_from_address(const uint8_t *data, size_t size);

char *fill_serial_param(char *buf, size_t size,
        accessory_id_t acc_id);




#endif
