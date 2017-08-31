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
#include "network.h"

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
    // printf("%s IP: %s", msg, sender);
}

ssize_t wrapIPpackageInEthernet(uint8_t *acc_buf, size_t size, uint8_t *sendbuf)
{
    struct iphdr *iph = (struct iphdr *)(acc_buf);
    
    // printIp("Source", iph->saddr);
    // printIp(" - Destination", iph->daddr);
    // puts("");
    
    if (g_haveSourceMac == 0) {
        g_sourceInterfaceMac = getAddrMac("tap0");
        g_destinationInterfaceMac = getAddrMac("br-lan");
        g_haveSourceMac = 1;
    }
    
    size_t buffSize = size + (sizeof(struct ether_header));
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

static void reportArp(uint8_t *acc_buf, size_t size)
{
    // ++++++++++++
    int i;
    uint8_t *ether_frame = acc_buf;
    arp_hdr *arphdr = (arp_hdr *)(acc_buf + 6 + 6 + 2);
    printf("\nEthernet frame header:\n");
    printf("Destination MAC (this node): ");
    for (i = 0; i < 5; i++)
    {
        printf("%02x:", ether_frame[i]);
    }
    printf("%02x\n", ether_frame[5]);
    printf("Source MAC: ");
    for (i = 0; i < 5; i++)
    {
        printf("%02x:", ether_frame[i + 6]);
    }
    printf("%02x\n", ether_frame[11]);
    // Next is ethernet type code (ETH_P_ARP for ARP).
    // http://www.iana.org/assignments/ethernet-numbers
    printf("Ethernet type code (2054 = ARP): %u\n", ((ether_frame[12]) << 8) + ether_frame[13]);
    printf("Ethernet data (ARP header):\n");
    printf("Hardware type (1 = ethernet (10 Mb)): %u\n", ntohs(arphdr->htype));
    printf("Protocol type (2048 for IPv4 addresses): %u\n", ntohs(arphdr->ptype));
    printf("Hardware (MAC) address length (bytes): %u\n", arphdr->hlen);
    printf("Protocol (IPv4) address length (bytes): %u\n", arphdr->plen);
    printf("Opcode (2 = ARP reply): %u\n", ntohs(arphdr->opcode));
    printf("Sender hardware (MAC) address: ");
    for (i = 0; i < 5; i++)
    {
        printf("%02x:", arphdr->sender_mac[i]);
    }
    printf("%02x\n", arphdr->sender_mac[5]);
    printf("Sender protocol (IPv4) address: %u.%u.%u.%u\n",
           arphdr->sender_ip[0], arphdr->sender_ip[1], arphdr->sender_ip[2], arphdr->sender_ip[3]);
    printf("Target (this node) hardware (MAC) address: ");
    for (i = 0; i < 5; i++)
    {
        printf("%02x:", arphdr->target_mac[i]);
    }
    printf("%02x\n", arphdr->target_mac[5]);
    printf("Target (this node) protocol (IPv4) address: %u.%u.%u.%u\n",
           arphdr->target_ip[0], arphdr->target_ip[1], arphdr->target_ip[2], arphdr->target_ip[3]);
}


void handleArpRequest(uint8_t *acc_buf, size_t size, int tunnelDev)
{
    
    // reportArp(acc_buf, size);
    
    int frame_length = 6 + 6 + 2 + ARP_HDRLEN;
    arp_hdr *sourceArpRequest = (arp_hdr *)(acc_buf + 6 + 6 + 2);
    arp_hdr arphdr_resp;

    uint8_t *ether_frame;
    ether_frame = allocate_ustrmem(frame_length);

    memcpy(&arphdr_resp.sender_mac, g_sourceInterfaceMac.ifr_hwaddr.sa_data, 6 * sizeof(uint8_t));
    memcpy(&arphdr_resp.sender_ip, sourceArpRequest->target_ip, 4 * sizeof(uint8_t));
    memcpy(&arphdr_resp.target_mac, sourceArpRequest->sender_mac, 6 * sizeof(uint8_t));
    memcpy(&arphdr_resp.target_ip, sourceArpRequest->sender_ip, 4 * sizeof(uint8_t));

    // for (i = 0; i < 6; i++)
    // {
    //     printf("%02x:", dst_mac[i]);
    // }
    // printf("\n");
    arphdr_resp.htype = htons(1);
    arphdr_resp.ptype = htons(ETH_P_IP);
    arphdr_resp.hlen = 6;
    arphdr_resp.plen = 4;
    arphdr_resp.opcode = htons(ARPOP_RESPONSE);

    // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (ARP header)

    // Destination and Source MAC addresses
    memcpy(ether_frame, sourceArpRequest->sender_mac, 6 * sizeof(uint8_t));
    memcpy(ether_frame + 6, g_sourceInterfaceMac.ifr_hwaddr.sa_data, 6 * sizeof(uint8_t));

    // Next is ethernet type code (ETH_P_ARP for ARP).
    // http://www.iana.org/assignments/ethernet-numbers
    ether_frame[12] = ETH_P_ARP / 256;
    ether_frame[13] = ETH_P_ARP % 256;

    // Next is ethernet frame data (ARP header).

    // ARP header
    memcpy(ether_frame + ETH_HDRLEN, &arphdr_resp, ARP_HDRLEN * sizeof(uint8_t));

    // reportArp(ether_frame, IP_MAXPACKET);

    int res = send_network_packet(ether_frame, frame_length);

    free(ether_frame);
}

// Allocate memory for an array of chars.
char * allocate_strmem(int len)
{
    void *tmp;

    if (len <= 0)
    {
        fprintf(stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
        exit(EXIT_FAILURE);
    }

    tmp = (char *)malloc(len * sizeof(char));
    if (tmp != NULL)
    {
        memset(tmp, 0, len * sizeof(char));
        return (tmp);
    }
    else
    {
        fprintf(stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
        exit(EXIT_FAILURE);
    }
}

// Allocate memory for an array of unsigned chars.
uint8_t * allocate_ustrmem(int len)
{
    void *tmp;

    if (len <= 0)
    {
        fprintf(stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
        exit(EXIT_FAILURE);
    }

    tmp = (uint8_t *)malloc(len * sizeof(uint8_t));
    if (tmp != NULL)
    {
        memset(tmp, 0, len * sizeof(uint8_t));
        return (tmp);
    }
    else
    {
        fprintf(stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
        exit(EXIT_FAILURE);
    }
}
