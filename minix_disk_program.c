#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#define BLOCK_SIZE 1024
#define MAX_FILENAME 60

// Global variables
int minix_fd = -1; // File descriptor for the mounted Minix disk image

// Superblock structure
struct superblock {
    unsigned short n_inodes;
    unsigned short n_zones;
    unsigned short imap_blocks;
    unsigned short zmap_blocks;
    unsigned short first_data_zone;
    unsigned short log_zone_size;
    unsigned long max_size;
    unsigned short magic;
    unsigned short state;
    unsigned long zones;
};

// Inode structure
struct inode {
    unsigned short mode;
    unsigned short uid;
    unsigned long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
    unsigned short zone[9];
};

// Directory entry structure
struct dir_entry {
    unsigned int inode;
    char name[MAX_FILENAME];
};

// Function prototypes
void print_help();
void minimount(char *filename);
void miniumount();
void showsuper();
void traverse(int long_list);
void showzone(int zone_number);
void quit();
void handle_command(char *command);
void print_permissions(unsigned short mode);

int main() {
    char command[256];

    printf("Minix Disk Console. Type 'help' for available commands.\n");
    while (1) {
        printf("minix: ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = 0; // Remove trailing newline
        handle_command(command);
    }

    return 0;
}

void print_help() {
    printf("Available commands:\n");
    printf("  help                  Show this help message\n");
    printf("  minimount [file]      Mount a Minix disk image\n");
    printf("  miniumount            Unmount the Minix disk image\n");
    printf("  showsuper             Show superblock information\n");
    printf("  traverse [-l]         List root directory contents\n");
    printf("  showzone [number]     Show the content of a zone\n");
    printf("  showfile [filename]   Show the content of a file in the root directory (Bonus)\n");
    printf("  quit                  Exit the Minix console\n");
}


void minimount(char *filename) {
    if (minix_fd != -1) {
        printf("A Minix disk is already mounted. Please unmount it first.\n");
        return;
    }

    minix_fd = open(filename, O_RDONLY);
    if (minix_fd == -1) {
        perror("Error opening file");
        return;
    }

    printf("Minix disk image '%s' mounted successfully.\n", filename);
}

void miniumount() {
    if (minix_fd == -1) {
        printf("No Minix disk is currently mounted.\n");
        return;
    }

    close(minix_fd);
    minix_fd = -1;
    printf("Minix disk unmounted successfully.\n");
}

void show_super() {
    if (minix_fd == -1) {  // Check if the Minix disk is mounted
        printf("No Minix disk is currently mounted.\n");
        return;
    }

    unsigned char buffer[BLOCK_SIZE];  // Buffer to hold the superblock data

    // Move the file pointer to the superblock (it is typically at block 1 in Minix)
    if (lseek(minix_fd, BLOCK_SIZE, SEEK_SET) == -1 || read(minix_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Failed to read superblock");
        return;
    }

    // Print out the superblock information from the buffer
    printf("Superblock Information:\n");
    printf("  Number of inodes:       %u\n", *(unsigned short *)(buffer + 0));        // n_inodes
    printf("  Number of zones:        %u\n", *(unsigned short *)(buffer + 2));        // n_zones
    printf("  Number of imap blocks:  %u\n", *(unsigned short *)(buffer + 4));        // imap_blocks
    printf("  Number of zmap blocks:  %u\n", *(unsigned short *)(buffer + 6));        // zmap_blocks
    printf("  First data zone:        %u\n", *(unsigned short *)(buffer + 8));        // first_data_zone
    printf("  Log zone size:          %u\n", *(unsigned short *)(buffer + 10));       // log_zone_size
    printf("  Max size:               %lu\n", *(unsigned long *)(buffer + 12));      // max_size
    printf("  Magic:                  %x\n", *(unsigned short *)(buffer + 16));       // magic
    printf("  State:                  %u\n", *(unsigned short *)(buffer + 18));       // state
    printf("  Zones:                  %lu\n", *(unsigned long *)(buffer + 20));       // zones
}


void print_permissions(unsigned short mode) {
    char permissions[11] = {0};

    // File type
    permissions[0] = (mode & 0x4000) ? 'd' : '-'; // Directory or file

    // Owner permissions
    permissions[1] = (mode & 0x0100) ? 'r' : '-';
    permissions[2] = (mode & 0x0080) ? 'w' : '-';
    permissions[3] = (mode & 0x0040) ? 'x' : '-';

    // Group permissions
    permissions[4] = (mode & 0x0020) ? 'r' : '-';
    permissions[5] = (mode & 0x0010) ? 'w' : '-';
    permissions[6] = (mode & 0x0008) ? 'x' : '-';

    // Other permissions
    permissions[7] = (mode & 0x0004) ? 'r' : '-';
    permissions[8] = (mode & 0x0002) ? 'w' : '-';
    permissions[9] = (mode & 0x0001) ? 'x' : '-';

    printf("%s ", permissions);
}

void traverse(int long_list) {
    if (minix_fd == -1) {
        printf("No Minix disk is currently mounted.\n");
        return;
    }

    struct superblock sb;
    lseek(minix_fd, BLOCK_SIZE, SEEK_SET); // Read the superblock
    if (read(minix_fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("Error reading superblock");
        return;
    }

    // Inodes start immediately after the imap and zmap blocks
    off_t inode_table_offset = (2 + sb.imap_blocks + sb.zmap_blocks) * BLOCK_SIZE;

    // Read the root inode (typically inode #1)
    struct inode root_inode;
    lseek(minix_fd, inode_table_offset + sizeof(root_inode), SEEK_SET); // Skip inode 0
    if (read(minix_fd, &root_inode, sizeof(root_inode)) != sizeof(root_inode)) {
        perror("Error reading root inode");
        return;
    }

    // Check if the inode represents a directory
    if ((root_inode.mode & 0xF000) != 0x4000) {
        printf("Root inode is not a directory.\n");
        return;
    }

    // Read the first zone of the root directory
    char buffer[BLOCK_SIZE];
    lseek(minix_fd, root_inode.zone[0] * BLOCK_SIZE, SEEK_SET);
    if (read(minix_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error reading root directory zone");
        return;
    }

    // Parse directory entries
    for (int offset = 0; offset < BLOCK_SIZE; offset += sizeof(struct dir_entry)) {
        struct dir_entry *entry = (struct dir_entry *)(buffer + offset);

        if (entry->inode == 0) continue; // Skip empty entries

        if (long_list) {
            // Read the inode for more details
            struct inode file_inode;
            lseek(minix_fd, inode_table_offset + (entry->inode - 1) * sizeof(file_inode), SEEK_SET);
            if (read(minix_fd, &file_inode, sizeof(file_inode)) != sizeof(file_inode)) {
                perror("Error reading file inode");
                return;
            }

            // Print file type, permissions, size, and modification time
            print_permissions(file_inode.mode);
            printf("%u ", file_inode.uid);
            printf("%lu ", file_inode.size);

            char time_str[20];
            struct tm *timeinfo = localtime((time_t *)&file_inode.mtime);
            strftime(time_str, sizeof(time_str), "%b %d %Y", timeinfo);
            printf("%s ", time_str);
        }

        printf("%s\n", entry->name);
    }
}

void showzone(int zone_number) {
    if (minix_fd == -1) {
        printf("No Minix disk is currently mounted.\n");
        return;
    }

    char buffer[BLOCK_SIZE];
    lseek(minix_fd, zone_number * BLOCK_SIZE, SEEK_SET);
    if (read(minix_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error reading zone");
        return;
    }

    printf("Zone %d content:\n", zone_number);
    for (int i = 0; i < BLOCK_SIZE; i += 16) {
        printf("%08x  ", i);
        for (int j = 0; j < 16; j++) {
            printf("%c ", isprint(buffer[i + j]) ? buffer[i + j] : '.');
        }
        printf("\n");
    }
}
void showfile(char *filename) {
    if (minix_fd == -1) {
        printf("No Minix disk is currently mounted.\n");
        return;
    }

    struct superblock sb;
    lseek(minix_fd, BLOCK_SIZE, SEEK_SET); // Read the superblock
    if (read(minix_fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("Error reading superblock");
        return;
    }

    // Inodes start immediately after the imap and zmap blocks
    off_t inode_table_offset = (2 + sb.imap_blocks + sb.zmap_blocks) * BLOCK_SIZE;

    // Read the root inode (typically inode #1)
    struct inode root_inode;
    lseek(minix_fd, inode_table_offset + sizeof(root_inode), SEEK_SET); // Skip inode 0
    if (read(minix_fd, &root_inode, sizeof(root_inode)) != sizeof(root_inode)) {
        perror("Error reading root inode");
        return;
    }

    // Check if the inode represents a directory
    if ((root_inode.mode & 0xF000) != 0x4000) {
        printf("Root inode is not a directory.\n");
        return;
    }

    // Read the first zone of the root directory
    char buffer[BLOCK_SIZE];
    lseek(minix_fd, root_inode.zone[0] * BLOCK_SIZE, SEEK_SET);
    if (read(minix_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error reading root directory zone");
        return;
    }

    // Parse directory entries to find the target file
    for (int offset = 0; offset < BLOCK_SIZE; offset += sizeof(struct dir_entry)) {
        struct dir_entry *entry = (struct dir_entry *)(buffer + offset);

        if (entry->inode == 0) continue; // Skip empty entries

        if (strcmp(entry->name, filename) == 0) {
            // File found, now read its inode
            struct inode file_inode;
            lseek(minix_fd, inode_table_offset + (entry->inode - 1) * sizeof(file_inode), SEEK_SET);
            if (read(minix_fd, &file_inode, sizeof(file_inode)) != sizeof(file_inode)) {
                perror("Error reading file inode");
                return;
            }

            // Check that the inode corresponds to a regular file
            if ((file_inode.mode & 0xF000) != 0x8000) {
                printf("%s is not a regular file.\n", filename);
                return;
            }

            // Now read the file data based on its zones
            printf("File contents of '%s':\n", filename);
            for (int i = 0; i < 9; i++) {
                if (file_inode.zone[i] == 0) break;  // No more zones
                lseek(minix_fd, file_inode.zone[i] * BLOCK_SIZE, SEEK_SET);
                if (read(minix_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
                    perror("Error reading file zone");
                    return;
                }
                // Print file content in hexadecimal format
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    printf("%02x ", buffer[j]);
                    if ((j + 1) % 16 == 0) {
                        printf("\n");
                    }
                }
                printf("\n");
            }
            return;
        }
    }

    // File not found
    printf("File '%s' not found in root directory.\n", filename);
}

void quit() {
    if (minix_fd != -1) {
        close(minix_fd);
    }
    printf("Exiting Minix console.\n");
    exit(0);
}

void handle_command(char *command) {
    char *args = strchr(command, ' ');
    if (args) {
        *args++ = '\0'; // Split the command and arguments
    }

    if (strcmp(command, "help") == 0) {
        print_help();
    } else if (strcmp(command, "minimount") == 0) {
        if (!args) {
            printf("Usage: minimount [file]\n");
        } else {
            minimount(args);
        }
    } else if (strcmp(command, "miniumount") == 0) {
        miniumount();
    } else if (strcmp(command, "showsuper") == 0) {
        show_super();
    } else if (strcmp(command, "traverse") == 0) {
        traverse(args && strcmp(args, "-l") == 0);
    } else if (strcmp(command, "showzone") == 0) {
        if (!args) {
            printf("Usage: showzone [number]\n");
        } else {
            showzone(atoi(args));
        }
    } else if (strcmp(command, "showfile") == 0) {
        if (!args) {
            printf("Usage: showfile [filename]\n");
        } else {
            showfile(args);
        }
    } else if (strcmp(command, "quit") == 0) {
        quit();
    } else {
        printf("Unknown command: %s\n", command);
    }
}
