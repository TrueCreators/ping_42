#include "../include/ft_ping.h"

int main(int argc, char *argv[]) {
    t_ping_options options;
    
    if (geteuid() != 0) {
        fprintf(stderr, "ping: operation not permitted\n");
        return 1;
    }

    // Initialize options with default values
    options.verbose = 0;
    options.packet_size = DEFAULT_PACKET_SIZE;
    options.ttl = 64;  // Default TTL value
    options.target = NULL;
    options.count = 0;

    if (parse_options(argc, argv, &options) != 0) {
        return 1;
    }

    // Check if target is localhost
    if (options.target && (strcmp(options.target, "localhost") == 0 || 
                          strcmp(options.target, "127.0.0.1") == 0)) {
        return ping_localhost(&options);
    }

    return send_ping(&options);
}