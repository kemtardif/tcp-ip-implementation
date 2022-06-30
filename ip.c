#include "ip.h"

int prepare_ip_packet(char *ip_packet, char *data, size_t data_size, 
                        u_int32_t ip_src,
                        u_int32_t ip_dst,
                        size_t *ip_size)
{
    u_int8_t version_ihl = (IPV4_VERSION << 4) | (IP_HEADER_WORDS);
    u_int8_t dscp_ecn = 1;
    u_int16_t identification = 2;
    u_int16_t flags_offset = 3;
    u_int8_t ttl = 10;
    u_int8_t protocol = UDP_PROTOCOL;
    u_int16_t checksum = 12345;
    u_int16_t total_length;
    //Initialize ip_packet to zero
    memset(ip_packet, 0, MAX_IP_SIZE);
    if(!sanitize_data_pckt_for_ip(data, data_size, ip_size))
        return 0;
    //Total length is changed if data_size is sanitized
    total_length = (IP_HEADER_WORDS * IP_WORD) + *ip_size;
    if(!append_ip_headers(ip_packet, data, 
                            version_ihl, 
                            dscp_ecn, 
                            total_length, 
                            identification, 
                            flags_offset, 
                            ttl, 
                            protocol, 
                            checksum, 
                            ip_src, 
                            ip_dst, 
                            ip_size))
        return 0;
    return 1;    
}


int get_ip_packet(struct ip_packet *ip_packet, char *ip_pckt, size_t ip_size)
{
    if(!ip_packet || !ip_pckt || ip_size < (IP_HEADER_WORDS * IP_WORD))
        return 0;

    //Copy version and internet header length
    memcpy(&ip_packet->version_ihl, ip_pckt, sizeof(u_int8_t));
    ip_pckt += sizeof(u_int8_t);
    //Copy dscp and ecn
    memcpy(&ip_packet->dscp_ecn, ip_pckt, sizeof(u_int8_t));
    ip_pckt += sizeof(u_int8_t);
    //Copy total length
    memcpy(&ip_packet->total_length, ip_pckt, sizeof(u_int16_t));
    ip_pckt += sizeof(u_int16_t);
    //Copy identification
    memcpy(&ip_packet->identification, ip_pckt, sizeof(u_int16_t));
    ip_pckt += sizeof(u_int16_t);
    //Copy flags and offset
    memcpy(&ip_packet->flags_offset, ip_pckt, sizeof(u_int16_t));
    ip_pckt += sizeof(u_int16_t);
    //Copy time-to-live
    memcpy(&ip_packet->ttl, ip_pckt, sizeof(u_int8_t));
    ip_pckt += sizeof(u_int8_t);
    //Copy protocol
    memcpy(&ip_packet->protocol, ip_pckt, sizeof(u_int8_t));
    ip_pckt += sizeof(u_int8_t);
    //Copy internet checksum
    memcpy(&ip_packet->checksum, ip_pckt, sizeof(u_int16_t));
    ip_pckt += sizeof(u_int16_t);
    //Copy source Ip
    memcpy(&ip_packet->ip_src, ip_pckt, sizeof(u_int32_t));
    ip_pckt += sizeof(u_int32_t);
    //Copy source Ip
    memcpy(&ip_packet->ip_dst, ip_pckt, sizeof(u_int32_t));
    ip_pckt += sizeof(u_int32_t);

    //ip_pckt now point to data. Data length is found from IHL
    ip_packet->data = (void *)ip_pckt;
    return 1;
}

int sanitize_data_pckt_for_ip(char *data_pckt, size_t data_size, size_t *ip_size)
{
    size_t size;
    if(!data_pckt || !data_size || !ip_size)
        return 0;
    if((size = data_size) > (MAX_IP_SIZE - (IP_HEADER_WORDS * IP_WORD)))
        size = (MAX_IP_SIZE - (IP_HEADER_WORDS * IP_WORD)); 
    *ip_size = size;
    return 1;
}

int append_ip_headers(char *ip_packet, char *data, 
                        u_int8_t version_ihl,
                        u_int8_t dscp_ecn,
                        u_int16_t total_length,
                        u_int16_t identification,
                        u_int16_t flags_offset,
                        u_int8_t ttl,
                        u_int8_t protocol,
                        u_int16_t checksum,
                        u_int32_t ip_src,
                        u_int32_t ip_dst,
                        size_t *ip_size)
{
    if(!ip_packet || !data || !ip_src || !ip_dst || !ip_size)
        return 0;

    memcpy(ip_packet, &version_ihl, sizeof(u_int8_t));
    ip_packet += sizeof(u_int8_t);

    memcpy(ip_packet, &dscp_ecn, sizeof(u_int8_t));
    ip_packet += sizeof(u_int8_t);
    //Copy total length
    memcpy(ip_packet, &total_length, sizeof(u_int16_t));
    ip_packet += sizeof(u_int16_t);
    //Copy identification
    memcpy(ip_packet, &identification, sizeof(u_int16_t));
    ip_packet += sizeof(u_int16_t);
    //Copy flags and offset
    memcpy(ip_packet, &flags_offset, sizeof(u_int16_t));
    ip_packet += sizeof(u_int16_t);
    //Copy time-to-live
    memcpy(ip_packet, &ttl, sizeof(u_int8_t));
    ip_packet += sizeof(u_int8_t);
    //Copy protocol
    memcpy(ip_packet, &protocol, sizeof(u_int8_t));
    ip_packet += sizeof(u_int8_t);
    //Copy internet checksum
    memcpy(ip_packet, &checksum, sizeof(u_int16_t));
    ip_packet += sizeof(u_int16_t);
    //Copy source Ip
    memcpy(ip_packet, &ip_src, sizeof(u_int32_t));
    ip_packet += sizeof(u_int32_t);
    //Copy source Ip
    memcpy(ip_packet, &ip_dst, sizeof(u_int32_t));
    ip_packet += sizeof(u_int32_t);

    //Finally add data and add ip header length to sending size
    memcpy(ip_packet, data, *ip_size);
    *ip_size += (IP_HEADER_WORDS * IP_WORD);
    return 1;
}

int L2_pass_to_L3_ip(struct interface *rcv_itf, char * packet, size_t pckt_size)
{
    struct ip_packet ip_packet;
    char src[20], dst[20];
    if(!rcv_itf || !packet || !pckt_size)
        return 0;


    if(!get_ip_packet(&ip_packet, packet, pckt_size))
    {
        printf("Could not remove IP headers. Drop packet\n");
        return 0;
    }
    if((ip_packet.version_ihl >> 4) != IPV4_VERSION)
        printf("Ip packet not IPV4. Drop packet.\n");
        
    printf("Version : %hhu\n", (ip_packet.version_ihl >> 4) & 0xF);
    printf("IHL : %hhu\n", ip_packet.version_ihl & 0xF);
    printf("DSCP : %hhu\n", (ip_packet.dscp_ecn >> 4) & 0xF);
    printf("ECN : %hhu\n", ip_packet.version_ihl & 0xF);
    printf("TOTAL LENGTH : %hu\n", ip_packet.total_length);
    printf("IDENTIFICATION : %hu\n", ip_packet.identification);
    printf("FLAGS AND OFFSET : %hu\n", ip_packet.flags_offset);
    printf("TTL : %hhu\n", ip_packet.ttl);
    printf("PROTOCOL : %hhu\n", ip_packet.protocol);
    printf("CHECKSUM : %hu\n", ip_packet.checksum);
    ip_to_string(src, 20, ip_packet.ip_src);
    printf("IP SOURCE : %s\n", src);
    ip_to_string(dst, 20, ip_packet.ip_dst);
    printf("IP DESTINATION : %s\n", dst);
    printf("DATA : %s\n", (char *)ip_packet.data);
    return 1;
  
}
