#include "../include/ft_ping.h"

void print_help(const char *program_name) {
    printf("Usage: %s [options] destination\n", program_name);
    printf("\nOptions:\n");
    printf("  -v                 verbose output\n");
    printf("  -c count           stop after sending count packets\n");
    printf("  -s packetsize      specify the number of data bytes to be sent\n");
    printf("  --ttl ttl          specify the IP Time To Live\n");
    printf("  -?                 print this help message\n");
}

int parse_options(int argc, char *argv[], t_ping_options *options) {
    int i;

    // Initialize options with defaults
    options->verbose = 0;
    options->count = 0;  // 0 means ping indefinitely
    options->packet_size = DEFAULT_PACKET_SIZE;
    options->ttl = 64;  // Default TTL value of 64 instead of using a define
    options->target = NULL;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-v") == 0) {
                options->verbose = 1;
            }
            else if (strcmp(argv[i], "-?") == 0) {
                print_help(argv[0]);
                exit(0);
            }
            else if (strcmp(argv[i], "-c") == 0) {
                if (++i >= argc) {
                    fprintf(stderr, "ft_ping: option requires an argument -- 'c'\n");
                    return -1;
                }
                options->count = atoi(argv[i]);
                if (options->count <= 0) {
                    fprintf(stderr, "ft_ping: bad number of packets to transmit.\n");
                    return -1;
                }
            }
            else if (strcmp(argv[i], "-s") == 0) {
                if (++i >= argc) {
                    fprintf(stderr, "ft_ping: option requires an argument -- 's'\n");
                    return -1;
                }
                int size = atoi(argv[i]);
                if (size <= 0) {
                    fprintf(stderr, "ft_ping: invalid packet size: %d\n", size);
                    return -1;
                }
                options->packet_size = size;
                if ((size_t)size > MAX_PACKET_SIZE - sizeof(struct icmphdr)) {
                    fprintf(stderr, "ft_ping: packet size too large: %d\n", size);
                    return -1;
                }
            }
            else if (strcmp(argv[i], "--ttl") == 0) {  // Changed from -t to --ttl
                if (++i >= argc) {
                    fprintf(stderr, "ft_ping: option requires an argument -- 'ttl'\n");
                    return -1;
                }
                options->ttl = atoi(argv[i]);
                if (options->ttl <= 0 || options->ttl > 255) {
                    fprintf(stderr, "ft_ping: invalid TTL: %d\n", options->ttl);
                    return -1;
                }
            }
            else {
                fprintf(stderr, "ft_ping: invalid option -- '%s'\n", argv[i]);
                return -1;
            }
        }
        else {
            if (options->target != NULL) {
                fprintf(stderr, "ft_ping: only one target host allowed\n");
                return -1;
            }
            options->target = argv[i];
        }
    }

    if (options->target == NULL) {
        fprintf(stderr, "ft_ping: missing host operand\n");
        return -1;
    }

    return 0;
}