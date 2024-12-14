#include "../include/ft_ping.h"

static volatile sig_atomic_t g_stop_ping = 0;
static int sock_fd = -1;

void handle_signal(int signum) {
    if (signum == SIGINT) {
        g_stop_ping = 1;
        if (sock_fd != -1) {
            shutdown(sock_fd, SHUT_RDWR);
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
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

double get_time_ms(struct timeval *t) {
    return (t->tv_sec * 1000.0 + t->tv_usec / 1000.0);
}

void print_statistics(const t_ping_stats *stats, const char *target) {
    double loss_percent = 0;
    if (stats->packets_sent > 0) {
        loss_percent = ((stats->packets_sent - stats->packets_received) * 100.0) / stats->packets_sent;
    }

    printf("\n--- %s ping statistics ---\n", target);
    printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n",
           stats->packets_sent, stats->packets_received, loss_percent);

    if (stats->packets_received > 0) {
        double avg = stats->rtt_sum / stats->packets_received;
        double mdev = sqrt((stats->rtt_sum_sq / stats->packets_received) - (avg * avg));
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               stats->rtt_min, avg, stats->rtt_max, mdev);
    }
}

int send_ping(t_ping_options *options) {
    struct sockaddr_in dest_addr;
    struct icmphdr icmp_header;
    char packet[MAX_PACKET_SIZE];
    char recv_buffer[MAX_PACKET_SIZE];
    struct timeval start, end, timeout;
    t_ping_stats stats = {0};
    struct sigaction sa;
    int sequence = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int is_localhost = 0;
    struct hostent *host;

    // Initialize statistics
    stats.rtt_min = INFINITY;

    // Create raw socket
    sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0) {
        perror("ft_ping: socket");
        return -1;
    }

    // Set up signal handling
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    // Set up destination address
    host = gethostbyname(options->target);
    if (!host) {
        fprintf(stderr, "ft_ping: unknown host %s\n", options->target);
        close(sock_fd);
        return -1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    memcpy(&dest_addr.sin_addr, host->h_addr, host->h_length);

    // Check if target is localhost
    is_localhost = (dest_addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK) ||
                   strcmp(options->target, "localhost") == 0 ||
                   strcmp(options->target, "127.0.0.1") == 0);

    // Set socket options
    if (is_localhost) {
        int ttl = 64;  // Fixed TTL for localhost
        if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0) {
            perror("ft_ping: setsockopt TTL");
            close(sock_fd);
            return -1;
        }
    } else {
        if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &options->ttl, sizeof(options->ttl)) != 0) {
            perror("ft_ping: setsockopt TTL");
            close(sock_fd);
            return -1;
        }
    }

    // Set receive timeout
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("ft_ping: setsockopt SO_RCVTIMEO");
        close(sock_fd);
        return -1;
    }

    // Show initial message
    if (options->verbose) {
        printf("PING %s (%s): %d data bytes, id 0x%x = %u\n",
               options->target,
               inet_ntoa(dest_addr.sin_addr),
               options->packet_size,
               getpid() & 0xFFFF,
               getpid() & 0xFFFF);
    } else {
        printf("PING %s (%s): %d data bytes\n",
               options->target,
               inet_ntoa(dest_addr.sin_addr),
               options->packet_size);
    }

    // Main ping loop
    while (!g_stop_ping && (options->count == 0 || sequence < options->count)) {
        memset(&icmp_header, 0, sizeof(icmp_header));
        memset(packet, 0, sizeof(packet));

        // Prepare ICMP header
        icmp_header.type = ICMP_ECHO;
        icmp_header.code = 0;
        icmp_header.un.echo.id = getpid() & 0xFFFF;
        icmp_header.un.echo.sequence = sequence;
        
        // Copy header to packet and fill data
        memcpy(packet, &icmp_header, sizeof(icmp_header));
        memset(packet + sizeof(icmp_header), 0x42, options->packet_size);

        // Calculate and set checksum
        icmp_header.checksum = calculate_checksum(packet, options->packet_size + sizeof(icmp_header));
        memcpy(packet, &icmp_header, sizeof(icmp_header));

        gettimeofday(&start, NULL);

        // Send packet
        ssize_t bytes_sent = sendto(sock_fd, packet, options->packet_size + sizeof(icmp_header), 0,
                                  (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (bytes_sent < 0) {
            if (errno == EINTR) break;
            perror("ft_ping: sendto");
            continue;
        }
        stats.packets_sent++;

        // Receive reply
        struct sockaddr_in recv_addr;
        ssize_t bytes_received;
        memset(recv_buffer, 0, sizeof(recv_buffer));
        bytes_received = recvfrom(sock_fd, recv_buffer, sizeof(recv_buffer), 0,
                                (struct sockaddr*)&recv_addr, &addr_len);

        if (bytes_received < 0) {
            if (errno != EINTR) {
                printf("Request timeout for icmp_seq=%d\n", sequence);
            }
            sequence++;
            continue;
        }

        gettimeofday(&end, NULL);

        struct iphdr *ip_header = (struct iphdr*)recv_buffer;
        struct icmphdr *icmp_reply = (struct icmphdr*)(recv_buffer + (ip_header->ihl * 4));

        // For localhost, both ECHO and ECHOREPLY are valid responses
        int valid_reply = (icmp_reply->type == ICMP_ECHOREPLY) ||
                         (is_localhost && icmp_reply->type == ICMP_ECHO);

        if (valid_reply && 
            icmp_reply->un.echo.id == (getpid() & 0xFFFF) &&
            icmp_reply->un.echo.sequence == sequence) {
            double rtt = get_time_ms(&end) - get_time_ms(&start);
            stats.packets_received++;
            stats.rtt_sum += rtt;
            stats.rtt_sum_sq += rtt * rtt;
            stats.rtt_min = fmin(stats.rtt_min, rtt);
            stats.rtt_max = fmax(stats.rtt_max, rtt);

            printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                   bytes_received - sizeof(struct iphdr),
                   inet_ntoa(recv_addr.sin_addr),
                   sequence,
                   ip_header->ttl,
                   rtt);

            if (options->verbose) {
                printf("ICMP: type=%d code=%d id=0x%04x seq=0x%04x\n",
                       icmp_reply->type, icmp_reply->code,
                       ntohs(icmp_reply->un.echo.id),
                       ntohs(icmp_reply->un.echo.sequence));
            }
        }

        sequence++;
        usleep(1000000);  // Wait 1 second between pings
    }

    print_statistics(&stats, options->target);
    close(sock_fd);
    sock_fd = -1;
    return 0;
}