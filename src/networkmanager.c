#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include "networkmanager.h"

int g_haveSourceMac = 0;
struct ifreq g_sourceInterfaceMac;
struct ifreq g_destinationInterfaceMac;

struct ifreq getAddrMac(interface) {
    puts("Getting MAC");
    int sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd == 0)
    {
        perror("Socket error");
        exit(1);
    }

    struct ifreq sourceInterfaceMac;

    /* Get the MAC address of the interface to send on */
    memset(&sourceInterfaceMac, 0, sizeof(struct ifreq));
    strncpy(sourceInterfaceMac.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sockfd, SIOCGIFHWADDR, &sourceInterfaceMac) < 0)
        perror("SIOCGIFHWADDR host interface");
    close(sockfd);

    printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[0],
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[1],
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[2],
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[3],
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[4],
           ((uint8_t *)&sourceInterfaceMac.ifr_hwaddr.sa_data)[5]);

    return sourceInterfaceMac;
}

void printIp(char *msg, void *addr)
{
    struct sockaddr_storage their_addr;
    char sender[INET6_ADDRSTRLEN];

    ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr = addr;
    inet_ntop(AF_INET, &((struct sockaddr_in *)&their_addr)->sin_addr, sender, sizeof sender);
    printf("%s IP: %s", msg, sender);
}

ssize_t wrapIPpackageInEthernet(uint8_t *acc_buf, size_t size, uint8_t *sendbuf)
{

    struct iphdr *iph = (struct iphdr *)(acc_buf);

    printIp("Source", iph->saddr);
    printIp(" - Destination", iph->daddr);
    puts("");

    if (g_haveSourceMac == 0) {
        g_sourceInterfaceMac = getAddrMac("tap0");
        g_destinationInterfaceMac = getAddrMac("br-lan");
        g_haveSourceMac = 1;
    }

    uint8_t buffSize = size + (sizeof(struct ether_header));
    struct ether_header *eh = (struct ether_header *)sendbuf;

    memset(sendbuf, 0, buffSize);

    eh->ether_shost[0] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[0];
    eh->ether_shost[1] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[1];
    eh->ether_shost[2] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[2];
    eh->ether_shost[3] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[3];
    eh->ether_shost[4] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[4];
    eh->ether_shost[5] = ((uint8_t *)&g_sourceInterfaceMac.ifr_hwaddr.sa_data)[5];
    
    eh->ether_dhost[0] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[0];
    eh->ether_dhost[1] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[1];
    eh->ether_dhost[2] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[2];
    eh->ether_dhost[3] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[3];
    eh->ether_dhost[4] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[4];
    eh->ether_dhost[5] = ((uint8_t *)&g_destinationInterfaceMac.ifr_hwaddr.sa_data)[5];

    /* Ethertype field */
    eh->ether_type = htons(ETH_P_IP);
    int headerLength = sizeof(struct ether_header);
    // memcpy(sendbuf[tx_len], data, size);
    for (int i = 0; i < size; i++)
    {
        sendbuf[i + headerLength] = acc_buf[i];
    }

    return buffSize;
}

