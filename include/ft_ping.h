#ifndef FT_PING_H
#define FT_PING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#define MAX_PACKET_SIZE 65507
#define DEFAULT_PACKET_SIZE 56
#define DEFAULT_TTL 64
#define DEFAULT_COUNT 4

typedef struct {
    int verbose;
    int count;
    int packet_size;
    int ttl;
    char *target;
} t_ping_options;

// Function prototypes
int parse_options(int argc, char *argv[], t_ping_options *options);
void print_help(const char *program_name);
int send_ping(t_ping_options *options);
void handle_signal(int signum);

double ft_sqrt(double number);

#endif // FT_PING_H