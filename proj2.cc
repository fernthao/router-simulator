/* Name: Thao Nguyen 
 Case Network ID: ttn60
 The filename: proj2.cc
 Date created: Sep 18, 2025
 Description: This program simulate how a router process packets coming through it. It takes 1 or 2 inputs depending on the mode, the packet trace file that contains IP header and the forwarding table file that contains rules to process the packets.

 There are 3 modes: 
 -p: print the packet trace file in the format:
	timestamp src_addr dst_addr checksum ttl
 -r: print the forwarding table in the format:
 	ip prefix_len interface
 -s: takes both a trace file and a forwarding table, and prints out the actions that the router take for each packet.

 It is submitted as Assignment 2 in the course CSDS 325: Computer Networks.
*/
#include <iostream>
#include <map>
#include <set>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <arpa/inet.h>
#include <netinet/ip.h>

#define ARG_PACKET_PRINT  0x1
#define ARG_FWD_TABLE_PRINT   0x2
#define ARG_SIMULATION 0x4
#define ARG_FWD_FILE 0x8
#define ARG_TRACE_FILE  0x10
#define FWD_TABLE_ENTRY_SZ 8
#define PACKET_SZ 28
#define CHECKSUM_VALID 1234
#define USE_DEFAULT_INTERFACE 0 
#define TTL_EXPIRED 0
#define NULL_ROUTE 0

unsigned short cmd_line_flags = 0;
char *fwd_file = NULL;
char *trace_file = NULL;

typedef struct {
    uint32_t ip;
    uint16_t prefix_len;
    uint16_t interface;
} fwd_table_entry;

typedef struct {
    uint32_t tv_sec;
    uint32_t tv_usec;
} timev;

typedef struct {
    timev timestamp;
    iphdr ip_header;
} packet;

void to_quad(uint32_t ip, uint8_t* quads) {
    quads[0] = (ip >> 24) & 0xFF;
    quads[1] = (ip >> 16) & 0xFF;
    quads[2] = (ip >> 8) & 0xFF;
    quads[3] = ip & 0xFF;
}

uint8_t get_first_8_bits(uint32_t ip) {
    return (ip >> 24) & 0xFF;
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
    // TODO refactor to reuse code for print and sim mode
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
    fwd_table_entry entry;
    FILE * file = fopen(fwd_file, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", fwd_file);
        exit(1);
    }

    // Read each entry in the forwarding table file
    // TODO refactor to reuse this code for simulation and print
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

int filesize(FILE * file) {
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

void simulation(char *fwd_filename, char *trace_filename) {
    int default_interface = -1;

    // Read files to arrays
    FILE * fwd_file = fopen(fwd_filename, "rb");
    FILE * trace_file = fopen(trace_filename, "rb");

    if (fwd_file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", fwd_filename);
        exit(1);
    }
    if (trace_file == nullptr) {
        fprintf(stderr, "Error: could not open file %s \n", trace_filename);
        exit(1);
    }
    
    int num_tbl_entries = filesize(fwd_file) / FWD_TABLE_ENTRY_SZ;
    fwd_table_entry fwd_table[num_tbl_entries];

    if (fread(fwd_table, FWD_TABLE_ENTRY_SZ, num_tbl_entries, fwd_file) != (size_t) num_tbl_entries) {
        fprintf(stderr, "Error: reading file unsuccessful");
    }

    std::set<uint32_t> duplicate_ips;
    std::map<uint8_t, fwd_table_entry> fwd_table_map; // ip first 8 bits -> fwd_table_entry

    for (int i = 0; i < num_tbl_entries; i++) {
        fwd_table_entry entry = fwd_table[i];
        // Convert to host byte order
        entry.ip = ntohl(entry.ip);
        entry.prefix_len = ntohs(entry.prefix_len);
        entry.interface = ntohs(entry.interface);

        // Check for default interface
        if (entry.ip == USE_DEFAULT_INTERFACE) {
            default_interface = entry.interface;
        }

        // Check for duplicates
        if (duplicate_ips.insert(entry.ip).second == false) {
            uint8_t quads[4];
            to_quad(entry.ip, quads);
            fprintf(stderr, "Error: duplicate ip in fwd_table: %u.%u.%u.%u\n", quads[0], quads[1], quads[2], quads[3]);
            exit(1);
        }
        // Map the first 8 bits of the ip to the fwd_table_entry
        fwd_table_map[get_first_8_bits(entry.ip)] = entry;

    }

    // Read and process each packet from the file
    packet packet;
    while (fread(&packet, PACKET_SZ, 1, trace_file) == 1){
        packet.timestamp.tv_sec = ntohl(packet.timestamp.tv_sec);
        packet.timestamp.tv_usec = ntohl(packet.timestamp.tv_usec);
        packet.ip_header.saddr = ntohl(packet.ip_header.saddr);
        packet.ip_header.daddr = ntohl(packet.ip_header.daddr);
        packet.ip_header.check = ntohs(packet.ip_header.check);
    
        packet.ip_header.ttl--;

        // 325 portion - prefix_len is always 8 
        // only match the first 8 bits of the destination ip
        uint8_t first_8_bits = get_first_8_bits(packet.ip_header.daddr);
        std::map<uint8_t, fwd_table_entry>::iterator match_iterator = fwd_table_map.find(first_8_bits);

        // Format timestamp
        std::ostringstream oss;
        oss << packet.timestamp.tv_sec << '.'
            << std::setw(6) << std::setfill('0') << packet.timestamp.tv_usec;
        std::string timestamp = oss.str();    
        
        // Process packet
        if (packet.ip_header.check != CHECKSUM_VALID) {
            std::cout << timestamp << " drop checksum" << std::endl;
            continue;
        }
        if (packet.ip_header.ttl == TTL_EXPIRED) {
            std::cout << timestamp << " drop expired" << std::endl;
            continue;
        }
        if (match_iterator != fwd_table_map.end() && match_iterator->second.interface == NULL_ROUTE) {
            std::cout << timestamp << " drop policy" << std::endl;
            continue;
        }
        if (match_iterator != fwd_table_map.end()) {
            std::string action = "send " + std::to_string(match_iterator->second.interface);
            std::cout << timestamp << " " << action << std::endl;
            continue;
        }
        // If not falling into any above rule AND default interface exists, send to default interface
        if (default_interface != -1) {
            std::string action = "default " + std::to_string(default_interface);
            std::cout << timestamp << " " << action << std::endl;
            continue;
        }
        // If not falling into any above rule, drop unknown
        std::cout << timestamp << " drop unknown" << std::endl;
        continue;
    }
    fclose(fwd_file);
    fclose(trace_file);
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
