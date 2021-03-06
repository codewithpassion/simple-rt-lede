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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "tun.h"
#include "network.h"
#include "utils.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define PLATFORM "linux"

#define IFACE_UP_SH_PATH "iface_up.sh"

/* tun stuff */
static int g_tun_fd = 0;
static pthread_t g_tun_thread;
static volatile bool g_tun_is_running = false;
static char g_tun_ip[32] = {0};

void _printIp(char *msg, void *addr)
{
    struct sockaddr_storage their_addr;
    char sender[INET6_ADDRSTRLEN];

    ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr = addr;
    inet_ntop(AF_INET, &((struct sockaddr_in *)&their_addr)->sin_addr, sender, sizeof sender);
    printf("%s IP: %s", msg, sender);
}

static inline void dump_addr_info(uint32_t addr, size_t size)
{
    uint32_t tmp = htonl(addr);
    printf("packet size = %zu, dest addr = %s, device id = %d\n", size,
           inet_ntoa(*(struct in_addr *)&tmp),
           addr & 0xff);
}


accessory_id_t get_acc_id_from_packet_offset(const uint8_t *data, size_t size, int offset, bool checkV4)
{
    uint32_t addr;
    unsigned int addr_offset = offset;

    /* only ipv4 supported */
    if (checkV4 && (size < 20 || ((*data >> 4) & 0xf) != 4))
    {
        goto end;
    }

    /* dest ip addr in LE */
    addr =
        (uint32_t)(data[addr_offset + 0] << 24) |
        (uint32_t)(data[addr_offset + 1] << 16) |
        (uint32_t)(data[addr_offset + 2] << 8) |
        (uint32_t)(data[addr_offset + 3] << 0);

printf("\n@@@@@@@@@@@@ offset %i @@@ %u\n", addr_offset, addr);

    if (NETWORK_ADDRESS(addr) == SIMPLERT_NETWORK_ADDRESS)
    {
        /* dump_addr_info(addr, size); */
        return ACC_ID_FROM_ADDR(addr);
    }

end:
    return 0;
}

accessory_id_t get_acc_id_from_address(const uint8_t *data, size_t size)
{
    uint32_t addr;
    /* dest ip addr in LE */
    addr =
        (uint32_t)(data[0] << 24) |
        (uint32_t)(data[1] << 16) |
        (uint32_t)(data[2] << 8) |
        (uint32_t)(data[3] << 0);

    printf("\n@@@@@@@@@@@@@@ %u\n", addr);

    if (NETWORK_ADDRESS(addr) == SIMPLERT_NETWORK_ADDRESS)
    {
        /* dump_addr_info(addr, size); */
        return ACC_ID_FROM_ADDR(addr);
    }

end:
    return 0;
}

accessory_id_t get_acc_id_from_packet(const uint8_t *data, size_t size, bool dst_addr)
{
    unsigned int addr_offset = (dst_addr ? 16 : 12);
    return get_acc_id_from_packet_offset(data, size, addr_offset, true);
}

static void *tun_thread_proc(void *arg)
{
    ssize_t nread;
    uint8_t acc_buf[ACC_BUF_SIZE];
    accessory_id_t id = 0;

    g_tun_is_running = true;
    struct ether_header *eh = (struct ether_header *)acc_buf;

    while (g_tun_is_running)
    {
        if ((nread = tun_read_ip_packet(g_tun_fd, acc_buf, sizeof(acc_buf))) > 0)
        {
            if (eh->ether_type == htons(ETH_P_ARP))
            {
                handleArpRequest(acc_buf, nread, g_tun_fd);
            }
            struct iphdr *iph = (struct iphdr *)(acc_buf + 14);

            // FROM DEVICE
            // printf("#### Proto: %i ", iph->protocol);
            // if (iph->protocol == 17) { //UDP
            //     printf(" UDP ");
            //     _printIp("Source", iph->saddr);
            //     _printIp(" - Dest ", iph->daddr);
            //     printf(" - S Port: %u", ((acc_buf[14 + 20]) << 8) + acc_buf[14 + 20 +1]); //ip header
            //     printf(" - D Port: %u", ((acc_buf[14 + 20 +2]) << 8) + acc_buf[14 + 20 +3]); //ip header
            //     printf("\n");
            // }
            int headerLength = sizeof(struct ether_header);
            int size = nread - headerLength;
            uint8_t ipPackage[size];
            for (int i = 0; i < size; i++)
            {
                ipPackage[i] = acc_buf[i + headerLength];
            }

            //send to all accessories except the static ones
            for (int accId = 2; accId < 255; accId++)
            {
                send_accessory_packet(ipPackage, size, accId);
            }
            // if ((id = get_acc_id_from_packet(ipPackage, size, true)) != 0)
            // {
            // }
            // else
            // {
            //     /* invalid packet received, ignore */
            // }
        }
        else if (nread < 0)
        {
            fprintf(stderr, "Error reading from tun: %s\n", strerror(errno));
            break;
        }
        else
        {
            /* EOF received */
            break;
        }
    }

    g_tun_is_running = false;

    return NULL;
}

/* FIXME */
static bool iface_up(const char *dev)
{
    char cmd[1024] = {0};
    char net_addr_str[32] = {0};
    char host_addr_str[32] = {0};

    simple_rt_config_t *config = get_simple_rt_config();

    uint32_t net_addr = htonl(SIMPLERT_NETWORK_ADDRESS);
    uint32_t host_addr = htonl(SIMPLERT_NETWORK_ADDRESS | 0x1);
    uint32_t mask = __builtin_popcount(NETWORK_ADDRESS(-1));

    snprintf(net_addr_str, sizeof(net_addr_str), "%s",
                inet_ntoa(*(struct in_addr *)&net_addr));

    snprintf(host_addr_str, sizeof(host_addr_str), "%s",
                inet_ntoa(*(struct in_addr *)&host_addr));

    snprintf(cmd, sizeof(cmd), "%s %s start %s %s %s %u %s %s\n",
                IFACE_UP_SH_PATH, PLATFORM, dev, net_addr_str, host_addr_str, mask,
                config->nameserver,
                config->interface);

    return system(cmd) == 0;
}

static bool iface_down(void)
{
    char cmd[1024] = {0};

    snprintf(cmd, sizeof(cmd), "%s %s stop",
                IFACE_UP_SH_PATH, PLATFORM);

    return system(cmd) == 0;
}

bool start_network(void)
{
    int tun_fd = 0;
    char tun_name[IFNAMSIZ] = {0};

    if (g_tun_is_running)
    {
        fprintf(stderr, "Network already started!\n");
        return false;
    }

    puts("starting network");

    if (!is_tun_present())
    {
        fprintf(stderr, "Tun dev is not present. Is kernel module loaded?\n");
        return false;
    }

    if ((tun_fd = tun_alloc(tun_name, sizeof(tun_name))) < 0)
    {
        perror("tun_alloc failed");
        return false;
    }

    if (!iface_up(tun_name))
    {
        fprintf(stderr, "Unable set iface %s up\n", tun_name);
        close(tun_fd);
        return false;
    }

    g_tun_fd = tun_fd;
    printf("%s interface configured!\n", tun_name);

    pthread_create(&g_tun_thread, NULL, tun_thread_proc, NULL);

    return true;
}

void stop_network(void)
{
    if (g_tun_is_running)
    {
        puts("stopping network");
        g_tun_is_running = false;
        pthread_cancel(g_tun_thread);
        pthread_join(g_tun_thread, NULL);
        iface_down();
    }

    if (g_tun_fd)
    {
        close(g_tun_fd);
        g_tun_fd = 0;
    }
}

ssize_t send_network_packet(const uint8_t *data, size_t size)
{
    ssize_t nwrite;

    nwrite = tun_write_ip_packet(g_tun_fd, data, size);
    if (nwrite < 0)
    {
        fprintf(stderr, "Error writing into tun: %s\n",
                strerror(errno));
        return -1;
    }

    return nwrite;
}

char *fill_serial_param(char *buf, size_t size, accessory_id_t id)
{
    simple_rt_config_t *config = get_simple_rt_config();
    uint32_t addr = htonl(SIMPLERT_NETWORK_ADDRESS | id);

    snprintf(buf, size, "%s,%s",
                inet_ntoa(*(struct in_addr *)&addr),
                config->nameserver);

    return buf;
}

