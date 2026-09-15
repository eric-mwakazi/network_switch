#include "define.h"

// Number of switch ports and simulated NICs connected to them.
#define NIC_COUNT 4

// Structure for a MAC address table entry
typedef struct MacEntry {
    char mac[18];
    int port;
    struct MacEntry* next;
} MacEntry;

// Structure for a simulated Ethernet Frame
typedef struct {
    char src_mac[18];
    char dest_mac[18];
    int ingress_port;
    char payload[128];
} EthernetFrame;

// A virtual NIC represents one host connected to one switch port.
typedef struct {
    char name[8];
    char host_mac[18];
    int port;
    unsigned long received_frames;
} SimulatedNic;

// Global pointer for the dynamic MAC address table
MacEntry* mac_table = NULL;

// Static topology used by the simulation.
SimulatedNic nics[NIC_COUNT] = {
    {"nic1", "00:AA:BB:CC:DD:01", 1, 0},
    {"nic2", "00:AA:BB:CC:DD:02", 2, 0},
    {"nic3", "00:AA:BB:CC:DD:03", 3, 0},
    {"nic4", "00:AA:BB:CC:DD:04", 4, 0}
};

/* 
 * Function to update or add an entry to the MAC table (Learning Phase)
 */
void learn_mac(const char* mac, int port) {
    MacEntry* current = mac_table;

    // Search if the MAC address already exists
    while (current != NULL) {
        if (strcmp(current->mac, mac) == 0) {
            if (current->port != port) {
                printf("[LEARN] MAC %s moved from Port %d to Port %d\n", mac, current->port, port);
                current->port = port; // Update port if it changed
            }
            return;
        }
        current = current->next;
    }

    // If not found, create a new entry and insert it at the beginning
    MacEntry* new_entry = (MacEntry*)malloc(sizeof(MacEntry));
    if (!new_entry) {
        perror("Failed to allocate memory for MAC table");
        exit(EXIT_FAILURE);
    }
    strncpy(new_entry->mac, mac, 18);
    new_entry->port = port;
    new_entry->next = mac_table;
    mac_table = new_entry;

    printf("[LEARN] Dynamic Entry Added: %s on Port %d\n", mac, port);
}

// Function to find the destination port (Forwarding Phase)
// Returns -1 if the destination is unknown (requires flooding)
int lookup_mac(const char* mac) {
    MacEntry* current = mac_table;
    while (current != NULL) {
        if (strcmp(current->mac, mac) == 0) {
            return current->port;
        }
        current = current->next;
    }
    return -1; 
}

SimulatedNic* find_nic_by_name(const char* name) {
    for (int i = 0; i < NIC_COUNT; i++) {
        if (strcmp(nics[i].name, name) == 0) {
            return &nics[i];
        }
    }
    return NULL;
}

// Simulate placing a frame on an egress NIC and the connected host receiving it.
void transmit_to_nic(int port, const EthernetFrame* frame) {
    for (int i = 0; i < NIC_COUNT; i++) {
        if (nics[i].port != port) {
            continue;
        }

        nics[i].received_frames++;
        printf("[TX] switch -> %s | %s -> %s | Data: %s\n",
               nics[i].name, frame->src_mac, frame->dest_mac, frame->payload);
        if (strcmp(frame->dest_mac, nics[i].host_mac) == 0 ||
            strcmp(frame->dest_mac, "FF:FF:FF:FF:FF:FF") == 0) {
            printf("[HOST %s] Accepted frame\n", nics[i].name);
        } else {
            printf("[HOST %s] Ignored frame for another MAC\n", nics[i].name);
        }
        return;
    }
}

// Unknown and broadcast traffic leaves every port except the ingress port.
void flood_frame(const EthernetFrame* frame) {
    for (int i = 0; i < NIC_COUNT; i++) {
        if (nics[i].port != frame->ingress_port) {
            transmit_to_nic(nics[i].port, frame);
        }
    }
}

// Function to process an incoming frame
void process_frame(EthernetFrame frame) {
    printf("\n[RX] nic%d -> switch | %s -> %s | Data: %s\n",
           frame.ingress_port, frame.src_mac, frame.dest_mac, frame.payload);

    // 1. Learning Phase
    learn_mac(frame.src_mac, frame.ingress_port);

    // 2. Forwarding / Filtering Phase
    // Check if it's a broadcast frame
    if (strcmp(frame.dest_mac, "FF:FF:FF:FF:FF:FF") == 0) {
        printf("[SWITCH] Broadcast: flooding all NICs except nic%d\n", frame.ingress_port);
        flood_frame(&frame);
        return;
    }

    int dest_port = lookup_mac(frame.dest_mac);

    if (dest_port == -1) {
        // Destination MAC unknown
        printf("[SWITCH] Unknown destination: flooding all NICs except nic%d\n", frame.ingress_port);
        flood_frame(&frame);
    } else if (dest_port == frame.ingress_port) {
        // Destination is on the same port as source (Filtering)
        printf("[SWITCH] Destination is on ingress NIC: frame filtered\n");
    } else {
        // Destination MAC known (Unicast Forwarding)
        printf("[SWITCH] Known destination: forwarding only to nic%d\n", dest_port);
        transmit_to_nic(dest_port, &frame);
    }
}

// Helper function to print the current state of the MAC Table
void print_mac_table() {
    printf("\n======= MAC ADDRESS TABLE =======\n");
    printf("%-20s | %-4s\n", "MAC Address", "Port");
    printf("---------------------------------\n");
    MacEntry* current = mac_table;
    if (current == NULL) {
        printf("(Table is empty)\n");
    }
    while (current != NULL) {
        printf("%-20s | %-4d\n", current->mac, current->port);
        current = current->next;
    }
    printf("=================================\n");
}

void print_nics(void) {
    printf("\n======= SIMULATED NICs =======\n");
    for (int i = 0; i < NIC_COUNT; i++) {
        printf("%-4s | Port %d | Host %s | Received %lu\n",
               nics[i].name, nics[i].port, nics[i].host_mac,
               nics[i].received_frames);
    }
    printf("==============================\n");
}

void print_help(void) {
    printf("Commands:\n");
    printf("  send <source-nic> <dest-nic|broadcast|MAC> <message>\n");
    printf("  nics     Show NICs and receive counters\n");
    printf("  table    Show the learned MAC table\n");
    printf("  help     Show this help\n");
    printf("  quit     Stop the simulation\n");
}

// Free allocated memory before exit
void free_mac_table() {
    MacEntry* current = mac_table;
    while (current != NULL) {
        MacEntry* next = current->next;
        free(current);
        current = next;
    }
}

int main(void) {
    char line[256];

    printf("Layer 2 switch simulation with %d NICs\n", NIC_COUNT);
    print_nics();
    print_help();

    while (true) {
        char command[16];
        char source_name[16];
        char destination[18];
        char payload[128];
        int fields;

        printf("\nswitch> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        // Keep the payload intact by parsing only the first three whitespace-separated fields.
        fields = sscanf(line, "%15s %15s %17s %127[^\n]",
                        command, source_name, destination, payload);
        if (fields < 1) {
            continue;
        }

        if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "nics") == 0) {
            print_nics();
        } else if (strcmp(command, "table") == 0) {
            print_mac_table();
        } else if (strcmp(command, "send") == 0) {
            SimulatedNic* source;
            SimulatedNic* destination_nic;
            EthernetFrame frame;

            if (fields != 4) {
                printf("Usage: send <source-nic> <dest-nic|broadcast|MAC> <message>\n");
                continue;
            }

            source = find_nic_by_name(source_name);
            if (source == NULL) {
                printf("Unknown source NIC '%s'. Use nic1 through nic%d.\n",
                       source_name, NIC_COUNT);
                continue;
            }

            // NIC names are a convenience alias for their connected host MAC address.
            destination_nic = find_nic_by_name(destination);
            strncpy(frame.src_mac, source->host_mac, sizeof(frame.src_mac));
            if (destination_nic != NULL) {
                strncpy(frame.dest_mac, destination_nic->host_mac, sizeof(frame.dest_mac));
            } else if (strcmp(destination, "broadcast") == 0) {
                strncpy(frame.dest_mac, "FF:FF:FF:FF:FF:FF", sizeof(frame.dest_mac));
            } else {
                strncpy(frame.dest_mac, destination, sizeof(frame.dest_mac));
            }
            strncpy(frame.payload, payload, sizeof(frame.payload));
            frame.src_mac[sizeof(frame.src_mac) - 1] = '\0';
            frame.dest_mac[sizeof(frame.dest_mac) - 1] = '\0';
            frame.payload[sizeof(frame.payload) - 1] = '\0';
            frame.ingress_port = source->port;
            process_frame(frame);
        } else {
            printf("Unknown command '%s'. Type 'help'.\n", command);
        }
    }

    free_mac_table();
    printf("\nSimulation stopped.\n");
    return 0;
}
