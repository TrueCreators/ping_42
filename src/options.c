#include "../include/ft_ping.h"

void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS] <hostname/IP>\n", program_name);
    printf("Options:\n");
    printf("  -v, --verbose      Enable verbose mode\n");
    printf("  -?, --help         Show this help message\n");
    printf("  -c, --count=N      Stop after sending N packets\n");
    printf("  -s, --size=N       Send packets of size N bytes\n");
    printf("  -t, --ttl=N        Set IP Time To Live\n");
}

int parse_options(int argc, char *argv[], t_ping_options *options) {
    // Initialize default options
    options->verbose = 0;
    options->count = DEFAULT_COUNT;
    options->packet_size = DEFAULT_PACKET_SIZE;
    options->ttl = DEFAULT_TTL;
    options->target = NULL;

    // Simple command-line parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = 1;
        } else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            exit(0);
        } else if (strncmp(argv[i], "-c", 2) == 0 || strncmp(argv[i], "--count=", 8) == 0) {
            char *value;
            if (strncmp(argv[i], "--count=", 8) == 0) {
                value = argv[i] + 8;
            } else if (i + 1 < argc) {
                value = argv[++i];
            } else {
                fprintf(stderr, "Missing count value\n");
                return -1;
            }
            options->count = atoi(value);
            if (options->count <= 0) {
                fprintf(stderr, "Invalid count value\n");
                return -1;
            }
        } else if (strncmp(argv[i], "-s", 2) == 0 || strncmp(argv[i], "--size=", 7) == 0) {
            char *value;
            if (strncmp(argv[i], "--size=", 7) == 0) {
                value = argv[i] + 7;
            } else if (i + 1 < argc) {
                value = argv[++i];
            } else {
                fprintf(stderr, "Missing size value\n");
                return -1;
            }
            options->packet_size = atoi(value);
            if (options->packet_size <= 0 || options->packet_size > MAX_PACKET_SIZE) {
                fprintf(stderr, "Invalid packet size\n");
                return -1;
            }
        } else if (strncmp(argv[i], "-t", 2) == 0 || strncmp(argv[i], "--ttl=", 6) == 0) {
            char *value;
            if (strncmp(argv[i], "--ttl=", 6) == 0) {
                value = argv[i] + 6;
            } else if (i + 1 < argc) {
                value = argv[++i];
            } else {
                fprintf(stderr, "Missing TTL value\n");
                return -1;
            }
            options->ttl = atoi(value);
            if (options->ttl <= 0 || options->ttl > 255) {
                fprintf(stderr, "Invalid TTL value\n");
                return -1;
            }
        } else if (!options->target) {
            options->target = argv[i];
        } else {
            fprintf(stderr, "Unknown option or multiple targets\n");
            return -1;
        }
    }

    if (!options->target) {
        fprintf(stderr, "No target specified\n");
        return -1;
    }

    return 0;
}