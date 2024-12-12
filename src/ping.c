#include "../include/ft_ping.h"
#define INFINITY (1.0 / 0.0)
#include <limits.h>

static volatile sig_atomic_t g_stop_ping = 0;
static int sock_fd = -1;  // Global socket descriptor for signal handler access

void handle_signal(int signum) {
    if (signum == SIGINT) {
        g_stop_ping = 1;
        
        // Close socket to interrupt blocking receive
        if (sock_fd != -1) {
            close(sock_fd);
            sock_fd = -1;
        }
    }
}

unsigned short calculate_checksum(void *b, int len) {
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

int send_ping(t_ping_options *options) {
    struct sockaddr_in target;
    struct icmphdr *icmp_header;
    char packet[MAX_PACKET_SIZE];
    struct timeval start, end;
    int packets_sent = 0, packets_recv = 0;
    struct sigaction sa;
    
    // Statistics tracking
    double rtt_sum = 0.0;
    double rtt_sum_sq = 0.0;
    double rtt_min = INFINITY;
    double rtt_max = 0.0;

    // Create raw socket
    sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    // Set up signal handling
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Set TTL
    if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &options->ttl, sizeof(options->ttl)) < 0) {
        perror("setsockopt");
        close(sock_fd);
        return -1;
    }

    // Set socket timeout to prevent indefinite blocking
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt timeout");
        close(sock_fd);
        return -1;
    }

    // Resolve hostname
    struct hostent *host = gethostbyname(options->target);
    if (!host) {
        herror("gethostbyname");
        close(sock_fd);
        return -1;
    }

    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    memcpy(&target.sin_addr, host->h_addr, host->h_length);

    printf("PING %s (%s) %d bytes of data.\n", 
           options->target, 
           inet_ntoa(target.sin_addr), 
           options->packet_size);

    // Remove count limit if not specified
    int max_packets = (options->count > 0) ? options->count : INT_MAX;

    while (packets_sent < max_packets && !g_stop_ping) {
        // Prepare ICMP packet
        memset(packet, 0, sizeof(packet));
        icmp_header = (struct icmphdr *)packet;

        icmp_header->type = ICMP_ECHO;
        icmp_header->code = 0;
        icmp_header->un.echo.id = getpid();
        icmp_header->un.echo.sequence = packets_sent;
        icmp_header->checksum = 0;

        // Add payload
        memset(packet + sizeof(struct icmphdr), 0xAA, options->packet_size - sizeof(struct icmphdr));

        // Calculate checksum
        icmp_header->checksum = calculate_checksum(packet, options->packet_size);

        // Get start time
        gettimeofday(&start, NULL);

        // Send packet
        ssize_t bytes_sent = sendto(sock_fd, packet, options->packet_size, 0, 
                                    (struct sockaddr *)&target, sizeof(target));
        if (bytes_sent < 0) {
            if (errno == EINTR) break;  // Interrupted by signal
            perror("sendto");
            continue;
        }

        packets_sent++;

        // Receive response
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        char recv_packet[MAX_PACKET_SIZE];

        ssize_t recv_bytes = recvfrom(sock_fd, recv_packet, sizeof(recv_packet), 0, 
                                      (struct sockaddr *)&from, &fromlen);
        
        gettimeofday(&end, NULL);

        if (recv_bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout, continue to next packet
                if (options->verbose) {
                    printf("Request timed out\n");
                }
                continue;
            }
            if (errno == EINTR) break;  // Interrupted by signal
            perror("recvfrom");
            continue;
        }

        // Calculate round trip time
        long long microseconds = (end.tv_sec - start.tv_sec) * 1000000 + 
                                 (end.tv_usec - start.tv_usec);
        double milliseconds = microseconds / 1000.0;

        // Check if it's an ICMP Echo Reply
        struct icmphdr *recv_icmp = (struct icmphdr *)(recv_packet + sizeof(struct iphdr));
        if (recv_icmp->type == ICMP_ECHOREPLY && 
            recv_icmp->un.echo.id == getpid() && 
            recv_icmp->un.echo.sequence == packets_sent - 1) {
            
            packets_recv++;
            
            // Update RTT statistics
            rtt_sum += milliseconds;
            rtt_sum_sq += milliseconds * milliseconds;
            
            if (milliseconds < rtt_min) rtt_min = milliseconds;
            if (milliseconds > rtt_max) rtt_max = milliseconds;

            printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", 
                   inet_ntoa(from.sin_addr), 
                   packets_sent - 1, 
                   options->ttl, 
                   milliseconds);
        }

        // Wait between packets
        usleep(1000000);  // 1 second
    }

    // Calculate standard deviation
    double rtt_avg = rtt_sum / packets_recv;
    double rtt_stddev = ft_sqrt((rtt_sum_sq / packets_recv) - (rtt_avg * rtt_avg));

    // Print summary
    printf("\n--- %s ping statistics ---\n", options->target);
    printf("%d packets transmitted, %d packets received, %d%% packet loss\n", 
           packets_sent, packets_recv, 
           packets_sent > 0 ? ((packets_sent - packets_recv) * 100) / packets_sent : 0);
    
    if (packets_recv > 0) {
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               rtt_min, rtt_avg, rtt_max, rtt_stddev);
    }

    close(sock_fd);
    sock_fd = -1;
    return 0;
}