#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <arpa/inet.h>
#include "ip.h" // TODO fix path when submitting to class server

#define ARG_PACKET_PRINT  0x1
#define ARG_FWD_TABLE_PRINT   0x2
#define ARG_SIMULATION 0x4
#define ARG_FWD_FILE 0x8
#define ARG_TRACE_FILE  0x10
#define FWD_TABLE_ENTRY_SZ 8
#define PACKET_SZ 28
#define CHECKSUM_VALID 1234

unsigned short cmd_line_flags = 0;
char *fwd_file = NULL;
char *trace_file = NULL;

struct fwd_table_entry {
    uint32_t ip;
    uint16_t prefix_len;
    uint16_t interface;
};

struct timev {
    uint32_t tv_sec;
    uint32_t tv_usec;
};

struct packet {
    timev timestamp;
    iphdr ip_header;
};

void to_quad(uint32_t ip, uint8_t* quads) {
    quads[0] = (ip >> 24) & 0xFF;
    quads[1] = (ip >> 16) & 0xFF;
    quads[2] = (ip >> 8) & 0xFF;
    quads[3] = ip & 0xFF;
}


void usage (char *progname)
{
    fprintf (stderr,"--------------- USAGE: ---------------\n");
    fprintf (stderr,"%s [-p] [-r] [-s] [-f forward_file] [-t trace_file]\n", progname);
    fprintf (stderr,"   -p    packet printing mode\n");
    fprintf (stderr,"   -r    forwarding table printing mode\n");
    fprintf (stderr,"   -s    simulation mode\n");
    fprintf (stderr,"   -f X  set forward file to \'X\'\n");
    fprintf (stderr,"   -t X  set trace file to \'X\'\n");
    exit (1);
}

// Parse command line arguments
void parseargs (int argc, char *argv [])
{
    int opt;

    while ((opt = getopt (argc, argv, "prsf:t:")) != -1)
    {
        switch (opt)
        {
            case 'p':
              cmd_line_flags |= ARG_PACKET_PRINT;
              break;
            case 'r':
              cmd_line_flags |= ARG_FWD_TABLE_PRINT;
              break;
            case 's':
              cmd_line_flags |= ARG_SIMULATION;
              break;
            case 'f':
              cmd_line_flags |= ARG_FWD_FILE;
              fwd_file = optarg;
              break;
            case 't':
              cmd_line_flags |= ARG_TRACE_FILE;
              trace_file = optarg;
              break;
            default:
              usage (argv [0]);
        }
    }
    if (cmd_line_flags == 0)
    {
        fprintf (stderr,"error: no command line option given\n");
        usage (argv [0]);
    }
}

void printPacket(char *trace_file) {
    packet packet;
    FILE * file = fopen(trace_file, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", trace_file);
        exit(1);
    }
    while (fread(&packet, PACKET_SZ, 1, file) == 1) {
        // Convert to host byte order
        packet.timestamp.tv_sec = ntohl(packet.timestamp.tv_sec);
        packet.timestamp.tv_usec = ntohl(packet.timestamp.tv_usec);
        packet.ip_header.saddr = ntohl(packet.ip_header.saddr);
        packet.ip_header.daddr = ntohl(packet.ip_header.daddr);
        packet.ip_header.check = ntohs(packet.ip_header.check);
        char checksum = packet.ip_header.check == CHECKSUM_VALID ? 'P' : 'F';

        // Convert ips to dotted-quad notation
        uint8_t squads[4];
        to_quad(packet.ip_header.saddr, squads);
        uint8_t dquads[4];
        to_quad(packet.ip_header.daddr, dquads);

        printf("%u.%06u %u.%u.%u.%u %u.%u.%u.%u %c %u\n", 
                packet.timestamp.tv_sec, packet.timestamp.tv_usec, 
                squads[0], squads[1], squads[2], squads[3], 
                dquads[0], dquads[1], dquads[2], dquads[3], 
                checksum, 
                packet.ip_header.ttl
            );
    }
    fclose(file);
}

void printForwardingTable(char *fwd_file) {
    // Print rules in forwarding table file in the following format:
    //          ip prefix_len interface
    // • ip: This is the IPv4 address from the forwarding table printed in dotted-quad notation. That is, four
    // unpadded decimal integers separated by periods (“.”). E.g., “192.168.10.54”.
    // • prefix len: The number of bits the router will use when comparing the IP address prefix in the rule
    // with the destination IP address in incoming packets.
    // • interface: The interface number on which to forward traffic matching the rule’s IP address and prefix.
    // Examples:
    // 10.0.0.0 8 1
    fwd_table_entry entry;
    FILE * file = fopen(fwd_file, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", fwd_file);
        exit(1);
    }

    // Read each entry in the forwarding table file
    while (fread(&entry, FWD_TABLE_ENTRY_SZ, 1, file) == 1) {
        // Turn fields into host byte order first
        uint32_t ip = ntohl(entry.ip);
        uint16_t prefix_len = ntohs(entry.prefix_len);
        uint16_t interface = ntohs(entry.interface);
        // Convert ip to dotted-quad notation
        uint8_t quads[4];
        to_quad(ip, quads);
        printf("%u.%u.%u.%u %u %u\n", quads[0], quads[1], quads[2], quads[3], prefix_len, interface);
    }
    fclose(file);
}

void simulation(char *fwd_file, char *trace_file) {
    std::cout << "Running simulation with forwarding file: " << fwd_file 
              << " and trace file: " << trace_file << std::endl;
    // Implementation of simulation goes here
}

int main (int argc, char *argv [])
{
    parseargs (argc,argv);

    if (cmd_line_flags & ARG_PACKET_PRINT) {
        if (!(cmd_line_flags & ARG_TRACE_FILE)) {
            fprintf (stderr,"error: -p requires -t trace_file\n");
            usage (argv [0]);
        }
        printPacket(trace_file);
    }
    else if (cmd_line_flags & ARG_FWD_TABLE_PRINT) {
        if (!(cmd_line_flags & ARG_FWD_FILE)) {
            fprintf (stderr,"error: -r requires -f forward_file\n");
            usage (argv [0]);
        }
        printForwardingTable(fwd_file);
    }
    else if (cmd_line_flags & ARG_SIMULATION) {
        if (!(cmd_line_flags & ARG_FWD_FILE) && !(cmd_line_flags & ARG_TRACE_FILE)) {
            fprintf (stderr,"error: -s requires -f forward_file and -t trace_file\n");
            usage (argv [0]);
        }
        if (!(cmd_line_flags & ARG_FWD_FILE)) {
            fprintf (stderr,"error: -s requires -f forward_file\n");
            usage (argv [0]);
        }
        if (!(cmd_line_flags & ARG_TRACE_FILE)) {
            fprintf (stderr,"error: -s requires -t trace_file\n");
            usage (argv [0]);
        }
        simulation(fwd_file, trace_file);
    }
    else
    {
        fprintf (stderr,"error: only one option at a time allowed\n");
        exit (1);
    }
    exit (0);
}
