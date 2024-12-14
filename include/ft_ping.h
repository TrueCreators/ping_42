#ifndef FT_PING_H
#define FT_PING_H

// Standard C Library headers
#include <stdio.h>      // For input/output functions
#include <stdlib.h>     // For general utilities
#include <string.h>     // For string manipulation
#include <unistd.h>     // For POSIX operating system API
#include <signal.h>     // For signal handling
#include <math.h>       // For mathematical functions

// Network related headers
#include <netdb.h>              // For network database operations
#include <sys/socket.h>         // For socket operations
#include <netinet/in.h>         // For Internet address family
#include <netinet/ip.h>         // For IP header structure
#include <netinet/ip_icmp.h>    // For ICMP header structure
#include <arpa/inet.h>          // For Internet operations
#include <sys/time.h>           // For time operations
#include <errno.h>              // For error handling

// Configuration constants
#define MAX_PACKET_SIZE 65507   // Maximum packet size (IP header + ICMP header + data)
#define DEFAULT_PACKET_SIZE 56  // Default size for ICMP data
#define DEFAULT_TTL 10         // Default Time To Live value
// Removed DEFAULT_COUNT as we now ping indefinitely by default

// Program options structure
typedef struct {
    int verbose;           // Verbose output flag
    int count;            // Number of packets to send (0 = infinite)
    int packet_size;      // Size of ICMP packet
    int ttl;             // Time To Live
    char *target;        // Target hostname/IP
} t_ping_options;

// Statistics structure
typedef struct {
    int packets_sent;     // Total packets sent
    int packets_received; // Total packets received
    double rtt_min;      // Minimum round-trip time
    double rtt_max;      // Maximum round-trip time
    double rtt_sum;      // Sum of round-trip times
    double rtt_sum_sq;   // Sum of squares of round-trip times
} t_ping_stats;

// Function prototypes

// Main ping function
int send_ping(t_ping_options *options);

// Options parsing and help
int parse_options(int argc, char *argv[], t_ping_options *options);
void print_help(const char *program_name);

int ping_localhost(t_ping_options *options);

// ICMP utilities
unsigned short calculate_checksum(void *b, int len);

// Signal handling
void handle_signal(int signum);

#endif // FT_PING_H