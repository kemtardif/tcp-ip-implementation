#include "arp.h"

int resolve_target_ip(struct interface *sender, u_int32_t target_ip, u_int8_t *target_mac)
{
    char ip_str[MAX_IP_STRING];
    u_int8_t *mac;
    int i;

    if(!sender || !target_ip)
        return RSLVD_ERR;
    
    if(!ip_to_string(ip_str, MAX_IP_STRING, target_ip))
        return RSLVD_ERR;
    if((mac = get_value(sender->arp_table, ip_str)) == NULL)
    {
        //MAc not resolved, will broadcast arp packet
        for(i = 0; i < MAC_SIZE; i++)
            target_mac[i] = 0xFF;
        return UNRSLVD;
    }
    //Mac address resolved
    for(i = 0; i < MAC_SIZE; i++)
        target_mac[i] = mac[i];
    return RSLVD;
}


int create_arp_structure(struct interface *requester, u_int32_t target_ip, u_int8_t *target_mac, int op, struct arp_packet *arp)
{
    if(!requester || ! target_ip || !op || !arp)
        return 0;
    
    arp->hrd = ETH_HRD_TYPE;
    arp->pro = IPV4_TYPE;
    arp->hln = sizeof(requester->mac_addr);
    arp->pln = sizeof(requester->ip.ip_addr);
    arp->op = op;

    arp->sha = malloc(arp->hln);
    arp->spa = malloc(arp->pln);
    arp->tha = malloc(arp->hln);
    arp->tpa = malloc(arp->pln);
    if(!arp->sha || !arp->spa
        || !arp->tha || !arp->tpa)
    {
        free_arp_request(arp);
        return 0;
    }
    //Set sender Hardware address
    memcpy(arp->sha, requester->mac_addr, arp->hln);
    //Set sender protocol address
    memcpy(arp->spa, &(requester->ip.ip_addr), arp->pln);
    //Set target hardware address
    if(target_mac)
        memcpy(arp->tha, target_mac, arp->hln);
    else
        memset(arp->tha, 0x0, arp->hln);
    //Set target protocol address
    memcpy(arp->tpa, &target_ip, arp->pln);
    return 1;    
}

int get_arp_packet(struct arp_packet *arp, char *packet, size_t pckt_size)
{
    char *pckt = packet;
    size_t rem_size = pckt_size;
    if(!arp || !pckt || pckt_size < ARP_MIN_SIZE)
        return 0;  

    memcpy(&arp->hrd, pckt, sizeof(u_int16_t));
    pckt += sizeof(u_int16_t);
    rem_size -= sizeof(u_int16_t);
    memcpy(&arp->pro, pckt, sizeof(u_int16_t));
    pckt += sizeof(u_int16_t);
    rem_size -= sizeof(u_int16_t);
    memcpy(&arp->hln, pckt, sizeof(u_int8_t));
    pckt += sizeof(u_int8_t);
    rem_size -= sizeof(u_int8_t);
    memcpy(&arp->pln, pckt, sizeof(u_int8_t));
    pckt += sizeof(u_int8_t);
    rem_size -= sizeof(u_int8_t);
    memcpy(&arp->op, pckt, sizeof(u_int16_t));
    pckt += sizeof(u_int16_t);
    rem_size -= sizeof(u_int16_t);

    if(rem_size < ((2 * arp->hln) + (2 * arp->pln)))
        return 0;

    arp->sha = malloc(arp->hln);
    arp->spa = malloc(arp->pln);
    arp->tha = malloc(arp->hln);
    arp->tpa = malloc(arp->pln);
    if(!arp->sha || !arp->spa
        || !arp->tha || !arp->tpa)
    {
        free_arp_request(arp);
        return 0;
    }

    memcpy(arp->sha, pckt, arp->hln);
    pckt += arp->hln;
    memcpy(arp->spa, pckt, arp->pln);
    pckt += arp->pln;
    memcpy(arp->tha, pckt, arp->hln);
    pckt += arp->hln;
    memcpy(arp->tpa, pckt, arp->pln);
    pckt += arp->pln;

    return 1;
}

int L2_pass_to_L3_arp(struct interface *rcv_itf, char *packet, size_t pckt_size)
{
    char str[20];
    int i;
    struct arp_packet arp;
    if(!rcv_itf || !packet || !pckt_size)
        return 0;
    if(!get_arp_packet(&arp, packet, pckt_size))
        return 0;
    //If interface not target, drop packet
    if(rcv_itf->ip.ip_addr != *(u_int32_t *)arp.tpa)
        printf("Interface %s not ARP target. Drop packet.\n", rcv_itf->name);
    else if(arp.op == REQ_OP)
        process_arp_request(rcv_itf, &arp);
    else if (arp.op == RESP_OP)
        process_arp_response(rcv_itf, &arp);

    //If request and has right IP, unicast arp response
    //If response and has right IP, update arp-table, send buffered packets
    free_arp_request(&arp);
}
void process_arp_request(struct interface *rcv_itf, struct arp_packet *req_arp)
{
    struct arp_packet arp_response;
    size_t rsp_size;
    char *response;
    char ethernet_pckt[MAX_ETH_SIZE];
    size_t eth_size;

    if(!rcv_itf || !req_arp)
        return;
    //Free entry coming from request source
    add_arp_entry(rcv_itf, *(u_int32_t *)req_arp->spa, (u_int8_t *)req_arp->sha);

    if((response = create_arp_packet(rcv_itf, *(u_int32_t *)req_arp->spa, (u_int8_t *)req_arp->sha, &rsp_size, RESP_OP)) == NULL)
        return;
    //Unicast send to requester
    if(!prepare_ethernet_packet(ethernet_pckt, response, rsp_size, rcv_itf->mac_addr, (u_int8_t *)req_arp->sha, ARP_TYPE, &eth_size))
            return;
    add_packet_to_send_queue(rcv_itf, ethernet_pckt, eth_size);
    printf("ARP response sent at interface %s\n", rcv_itf->name);   
    free(response);
}

void process_arp_response(struct interface *rcv_itf, struct arp_packet *arp)
{
    if(!rcv_itf || !arp)
        return;
    //Update arp table with response
    add_arp_entry(rcv_itf, *(u_int32_t *)arp->spa, (u_int8_t *)arp->sha);

    //Sent waiting packet
    send_waiting_packet(rcv_itf, *(u_int32_t *)arp->spa);
}

void *create_arp_packet(struct interface *requester, u_int32_t target_ip, 
                            u_int8_t *target_mac, size_t *arp_size, int op)
{
    struct arp_packet arp;
    char *packet, *pckt_ptr;
    size_t pckt_size;
    if(!requester || !target_ip || !arp_size)
        return 0;
    if(!create_arp_structure(requester, target_ip, target_mac, op, &arp))
        return 0;
    if(((pckt_size = get_arp_size(&arp)) == 0)
        || ((packet = pckt_ptr = malloc(pckt_size)) == NULL))
    {
        free_arp_request(&arp);
        return 0;
    }
    memcpy(packet, &arp.hrd, sizeof(u_int16_t));
    packet += sizeof(u_int16_t);
    memcpy(packet, &arp.pro, sizeof(u_int16_t));
    packet += sizeof(u_int16_t);
    memcpy(packet, &arp.hln, sizeof(u_int8_t));
    packet += sizeof(u_int8_t);
    memcpy(packet, &arp.pln, sizeof(u_int8_t));
    packet += sizeof(u_int8_t);
    memcpy(packet, &arp.op, sizeof(u_int16_t));
    packet += sizeof(u_int16_t);
    memcpy(packet, arp.sha, arp.hln);
    packet += arp.hln;
    memcpy(packet, arp.spa, arp.pln);
    packet += arp.pln;
    memcpy(packet, arp.tha, arp.hln);
    packet += arp.hln;
    memcpy(packet, arp.tpa, arp.pln);
    packet += arp.pln;
    *arp_size = pckt_size;

    free_arp_request(&arp);
    return pckt_ptr;    
}


int buffer_packet_arp(struct interface *sender, char *packet, size_t pckt_size, u_int32_t target_ip)
{
    int i;
    struct arp_buffer_item item;

    if(!sender || !packet || !pckt_size)
        return 0;
    for(i = 0; i < ARP_BUFFER_LENGTH; i++)
    {
        item = sender->arp_buffer[i];
        if(!sender->arp_buffer[i].target_ip)
            break;
    }
    if(i == ARP_BUFFER_LENGTH)
        return 0; //Buffer full

    sender->arp_buffer[i].target_ip = target_ip;
    if((sender->arp_buffer[i].packet = malloc(pckt_size)) == NULL)
        return 0;
    memcpy(sender->arp_buffer[i].packet, packet, pckt_size);
    sender->arp_buffer[i].pckt_size = pckt_size;
    return 1;   
}

void add_arp_entry(struct interface *itf, u_int32_t ip, u_int8_t *mac)
{
    char ip_str[MAX_IP_STRING];
    u_int8_t parts[IP_SIZE], *mac_entry;  

    if(!itf || !ip || !mac)
        return;

    parts[0] = (ip >> 24) & 0xFF;
    parts[1] = (ip >> 16) & 0xFF;
    parts[2] = (ip >> 8) & 0xFF;
    parts[3] = (ip >> 0) & 0xFF;
    if(snprintf(ip_str, MAX_IP_STRING, "%i.%i.%i.%i", 
                                parts[0],
                                parts[1],
                                parts[2],
                                parts[3]) == -1)
        return;
        
    if((mac_entry = malloc(MAC_SIZE * sizeof(u_int8_t))) == NULL)
        return;
    set_mac(mac_entry, mac);
    set_value(itf->arp_table, ip_str, (void *)mac_entry, FREE);    
}

void send_waiting_packet(struct interface *itf, u_int32_t ip)
{
    struct arp_buffer_item item;
    int i;
    char ethernet_pckt[MAX_ETH_SIZE];
    u_int8_t dstMac[MAC_SIZE];
    size_t eth_size;
    if(!itf || !ip)
        return;
    for(i = 0; i < ARP_BUFFER_LENGTH; i ++)
    {
        item = itf->arp_buffer[i];
        if(resolve_target_ip(itf, item.target_ip, dstMac) == RSLVD)
        {
            if(!prepare_ethernet_packet(ethernet_pckt, item.packet, item.pckt_size, itf->mac_addr, dstMac, IPV4_TYPE, &eth_size))
                continue;
            add_packet_to_send_queue(itf, ethernet_pckt, eth_size);
            printf("Packet in arp buffer sent at interface %s\n", itf->name);
            free(itf->arp_buffer[i].packet);
            itf->arp_buffer[i].target_ip = 0;
            itf->arp_buffer[i].pckt_size = 0;
            itf->arp_buffer[i].packet = NULL;
        }
    }
}
void free_arp_request(struct arp_packet *arp)
{
    if(!arp)
        return;
    if(arp->sha)
        free(arp->sha);
    if(arp->spa)
        free(arp->spa);
    if(arp->tha)
        free(arp->tha);
    if(arp->tpa)
        free(arp->tpa);
    return;
}

size_t get_arp_size(struct arp_packet *arp)
{
    size_t size = 0;
    if(!arp)
        return 0;

    size += (sizeof(u_int16_t) * 3);
    size += (sizeof(u_int8_t) * 2);
    size += (arp->hln * 2);
    size += (arp->pln * 2);
    return size;
} 
void print_arp(struct arp_packet *arp)
{
    char mac_str[MAX_MAC_STRING];
    char ip_str[MAX_IP_STRING];

    if(!arp)
        return;
    mac_to_string(mac_str, MAX_MAC_STRING, (u_int8_t *)arp->sha);
    printf("SENDING HA : %s\n", mac_str);
    ip_to_string(ip_str, MAX_IP_STRING, *(u_int32_t *)arp->spa);
    printf("SENDING PA : %s\n", ip_str);
    mac_to_string(mac_str, MAX_MAC_STRING, (u_int8_t *)arp->tha);
    printf("TARGET HA : %s\n", mac_str);
    ip_to_string(ip_str, MAX_IP_STRING, *(u_int32_t *)arp->tpa);
    printf("TARGET PA : %s\n", ip_str);
}