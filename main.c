#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
    char payload[64];
} EthernetFrame;

// Global pointer for the dynamic MAC address table
MacEntry* mac_table = NULL;

// Function to update or add an entry to the MAC table (Learning Phase)
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

// Function to process an incoming frame
void process_frame(EthernetFrame frame) {
    printf("\n--- Incoming Frame on Port %d ---\n", frame.ingress_port);
    printf("SRC: %s | DEST: %s | Data: %s\n", frame.src_mac, frame.dest_mac, frame.payload);

    // 1. Learning Phase
    learn_mac(frame.src_mac, frame.ingress_port);

    // 2. Forwarding / Filtering Phase
    // Check if it's a broadcast frame
    if (strcmp(frame.dest_mac, "FF:FF:FF:FF:FF:FF") == 0) {
        printf("[FORWARD] Broadcast Frame! Flooding to all ports except Port %d\n", frame.ingress_port);
        return;
    }

    int dest_port = lookup_mac(frame.dest_mac);

    if (dest_port == -1) {
        // Destination MAC unknown
        printf("[FORWARD] Destination MAC unknown. Flooding frame to all ports except Port %d\n", frame.ingress_port);
    } else if (dest_port == frame.ingress_port) {
        // Destination is on the same port as source (Filtering)
        printf("[FILTER] Destination port matches ingress port. Frame dropped.\n");
    } else {
        // Destination MAC known (Unicast Forwarding)
        printf("[FORWARD] Unicast Frame directly to Port %d\n", dest_port);
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

// Free allocated memory before exit
void free_mac_table() {
    MacEntry* current = mac_table;
    while (current != NULL) {
        MacEntry* next = current->next;
        free(current);
        current = next;
    }
}

int main() {
    // Array of mock sequential frames simulating network traffic
    EthernetFrame traffic[] = {
        {"00:AA:BB:CC:DD:01", "FF:FF:FF:FF:FF:FF", 1, "Hello Network (Broadcast)"}, // PC1 broadcasts
        {"00:AA:BB:CC:DD:02", "00:AA:BB:CC:DD:01", 2, "Reply to PC1 (Unicast)"},     // PC2 replies to PC1
        {"00:AA:BB:CC:DD:03", "00:AA:BB:CC:DD:02", 3, "Msg to PC2 (Unicast)"},      // PC3 sends to PC2 (PC2 is known)
        {"00:AA:BB:CC:DD:01", "00:AA:BB:CC:DD:03", 1, "Msg to PC3 (Unicast)"},      // PC1 sends to PC3 (PC3 is known)
        {"00:AA:BB:CC:DD:04", "00:AA:BB:CC:DD:05", 4, "Unknown to Unknown"},        // PC4 sends to an unknown PC5
        {"00:AA:BB:CC:DD:01", "00:AA:BB:CC:DD:02", 1, "Duplicate local frame"}       // PC1 attempts to reach PC2 on port 1
    };

    int total_frames = sizeof(traffic) / sizeof(traffic[0]);

    for (int i = 0; i < total_frames; i++) {
        process_frame(traffic[i]);
    }

    // Print final learned switch state
    print_mac_table();

    // Clean up memory
    free_mac_table();
    return 0;
}
