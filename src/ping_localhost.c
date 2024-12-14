#include "../include/ft_ping.h"

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 1

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// Changed to match project style and take options parameter
int ping_localhost(t_ping_options *options) {
    struct sockaddr_in addr;
    struct icmphdr *icmp;
    int sockfd, cnt = 0, ret;
    char packet[PACKET_SIZE];
    struct timeval start, end;
    t_ping_stats stats = {0};
    stats.rtt_min = INFINITY;
    
    // Create RAW socket for ICMP
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Prepare localhost address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("PING localhost (127.0.0.1): %ld data bytes\n", PACKET_SIZE - sizeof(struct icmphdr));

    // Use either specified count or default 4 for localhost
    int max_count = options->count > 0 ? options->count : 4;

    // Send packets
    while (cnt < max_count) {
        memset(packet, 0, sizeof(packet));
        icmp = (struct icmphdr *)packet;
        
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid() & 0xFFFF;
        icmp->un.echo.sequence = cnt;
        icmp->checksum = 0;
        icmp->checksum = checksum(icmp, sizeof(struct icmphdr));

        gettimeofday(&start, NULL);

        ret = sendto(sockfd, packet, sizeof(struct icmphdr), 0,
                    (struct sockaddr *)&addr, sizeof(addr));
        if (ret < 0) {
            perror("sendto");
            continue;
        }
        stats.packets_sent++;

        ret = recvfrom(sockfd, packet, sizeof(packet), 0, NULL, NULL);
        if (ret < 0) {
            perror("recvfrom");
            continue;
        }

        gettimeofday(&end, NULL);
        stats.packets_received++;

        double microseconds = (end.tv_sec - start.tv_sec) * 1000000.0 + 
                            (end.tv_usec - start.tv_usec);
        double milliseconds = microseconds / 1000.0;

        // Update statistics
        stats.rtt_sum += milliseconds;
        stats.rtt_sum_sq += milliseconds * milliseconds;
        stats.rtt_min = fmin(stats.rtt_min, milliseconds);
        stats.rtt_max = fmax(stats.rtt_max, milliseconds);

        printf("%d bytes from 127.0.0.1: icmp_seq=%d ttl=64 time=%.3f ms\n", 
               PACKET_SIZE, cnt, milliseconds);

        if (options->verbose) {
            printf("ICMP: type=%d code=%d id=0x%04x seq=0x%04x\n",
                   icmp->type, icmp->code,
                   ntohs(icmp->un.echo.id),
                   ntohs(icmp->un.echo.sequence));
        }

        cnt++;
        sleep(1);
    }

    // Print final statistics
    double loss_percent = 0;
    if (stats.packets_sent > 0) {
        loss_percent = ((stats.packets_sent - stats.packets_received) * 100.0) / stats.packets_sent;
    }

    printf("\n--- localhost ping statistics ---\n");
    printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n",
           stats.packets_sent, stats.packets_received, loss_percent);

    if (stats.packets_received > 0) {
        double avg = stats.rtt_sum / stats.packets_received;
        double mdev = sqrt((stats.rtt_sum_sq / stats.packets_received) - (avg * avg));
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               stats.rtt_min, avg, stats.rtt_max, mdev);
    }

    close(sockfd);
    return 0;
}