#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <arpa/inet.h>

#define ARG_PACKET_PRINT  0x1
#define ARG_FWD_TABLE_PRINT   0x2
#define ARG_SIMULATION 0x4
#define ARG_FWD_FILE 0x8
#define ARG_TRACE_FILE  0x10
#define FWD_TABLE_ENTRY_SZ 8

unsigned short cmd_line_flags = 0;
char *fwd_file = NULL;
char *trace_file = NULL;

struct ForwardingTableEntry {
    uint32_t ip;
    uint16_t prefix_len;
    uint16_t interface;
};

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
    std::cout << "Printing packets from trace file: " << trace_file << std::endl;
    // Implementation of packet printing goes here
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
    ForwardingTableEntry entry;
    FILE * file = fopen(fwd_file, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", fwd_file);
        exit(1);
    }

    // Read each entry in the forwarding table file
    while (fread(&entry, FWD_TABLE_ENTRY_SZ, 1, file) == 1) {
        // Turn fields into host byte order first
        uint32_t ip = ntohl(entry.ip);
        uint8_t quad1 = (ip >> 24) & 0xFF;
        uint8_t quad2 = (ip >> 16) & 0xFF;
        uint8_t quad3 = (ip >> 8) & 0xFF;
        uint8_t quad4 = ip & 0xFF;
        uint16_t prefix_len = ntohs(entry.prefix_len);
        uint16_t interface = ntohs(entry.interface);
        printf("%u.%u.%u.%u %u %u\n", quad1, quad2, quad3, quad4, prefix_len, interface);
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
