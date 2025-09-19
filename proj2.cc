#include <iostream>
#include <cstdlib>
#include <unistd.h>

#define ARG_PACKET_PRINT  0x1
#define ARG_FWD_TABLE_PRINT   0x2
#define ARG_SIMULATION 0x4
#define ARG_FWD_FILE 0x8
#define ARG_TRACE_FILE  0x10

unsigned short cmd_line_flags = 0;
char *fwd_file = NULL;
char *trace_file = NULL;

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
    std::cout << "Printing forwarding table from file: " << fwd_file << std::endl;
    // Implementation of forwarding table printing goes here
}

void simulation(char *fwd_file, char *trace_file) {
    std::cout << "Running simulation with forwarding file: " << fwd_file 
              << " and trace file: " << trace_file << std::endl;
    // Implementation of simulation goes here
}

int main (int argc, char *argv [])
{
    parseargs (argc,argv);

    if (cmd_line_flags == ARG_PACKET_PRINT) {
        if (!(cmd_line_flags & ARG_TRACE_FILE)) {
            fprintf (stderr,"error: -p requires -t trace_file\n");
            usage (argv [0]);
        }
        printPacket(trace_file);
    }
    else if (cmd_line_flags == ARG_FWD_TABLE_PRINT) {
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
