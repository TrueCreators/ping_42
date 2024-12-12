#include "../include/ft_ping.h"

int main(int argc, char *argv[]) {
    t_ping_options options;

    // Check for root privileges (needed for raw socket)
    if (geteuid() != 0) {
        fprintf(stderr, "ft_ping must be run with root privileges\n");
        return 1;
    }

    // Parse command-line options
    if (parse_options(argc, argv, &options) != 0) {
        return 1;
    }

    // Send ping
    return send_ping(&options);
}