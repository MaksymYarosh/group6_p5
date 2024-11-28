#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

// Constants
#define BLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
#define ROOT_DIR_START  /* Adjust to your filesystem structure */
#define INODE_TABLE_START  /* Adjust as needed */
#define DATA_BLOCK_START  /* Adjust as needed */

// Global variables
int disk_fd = -1; // File descriptor for the mounted Minix disk

// Function prototypes
void print_help();
void minimount(const char *filename);
void miniumount();
void showsuper();
void traverse(int long_list);
void showzone(int zone_number);
void showfile(const char *filename);
void read_block(int block_num, void *buffer);
void parse_permissions(mode_t mode, char *perm);

int main() {
    char command[256];

    printf("Minix Disk Program\n");
    printf("Type 'help' for a list of commands.\n");

    while (1) {
        printf("minix> ");
        if (!fgets(command, sizeof(command), stdin)) {
            perror("fgets failed");
            break;
        }

        // Remove trailing newline
        command[strcspn(command, "\n")] = 0;

        // Parse command
        char *cmd = strtok(command, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "minimount") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename) minimount(filename);
            else printf("Usage: minimount [image file]\n");
        } else if (strcmp(cmd, "miniumount") == 0) {
            miniumount();
        } else if (strcmp(cmd, "showsuper") == 0) {
            showsuper();
        } else if (strcmp(cmd, "traverse") == 0) {
            char *flag = strtok(NULL, " ");
            traverse(flag && strcmp(flag, "-l") == 0);
        } else if (strcmp(cmd, "showzone") == 0) {
            char *zone = strtok(NULL, " ");
            if (zone) showzone(atoi(zone));
            else printf("Usage: showzone [zone number]\n");
        } else if (strcmp(cmd, "showfile") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename) showfile(filename);
            else printf("Usage: showfile [filename]\n");
        } else if (strcmp(cmd, "quit") == 0) {
            miniumount();
            printf("Exiting Minix console.\n");
            break;
        } else {
            printf("Unknown command. Type 'help' for a list of commands.\n");
        }
    }

    return 0;
}

void print_help() {
    printf("Supported commands:\n");
    printf("  help                - Show this help message.\n");
    printf("  minimount [file]    - Mount a Minix disk image.\n");
    printf("  miniumount          - Unmount the Minix disk.\n");
    printf("  showsuper           - Display superblock information.\n");
    printf("  traverse [-l]       - List contents of the root directory.\n");
    printf("  showzone [zone]     - Display ASCII content of a specified zone.\n");
    printf("  showfile [file]     - Display the content of a file in the root directory.\n");
    printf("  quit                - Exit the console.\n");
}

void minimount(const char *filename) {
    if (disk_fd != -1) {
        printf("A disk is already mounted. Please unmount it first.\n");
        return;
    }
    disk_fd = open(filename, O_RDONLY);
    if (disk_fd == -1) {
        perror("Failed to open disk image");
    } else {
        printf("Disk image '%s' mounted successfully.\n", filename);
    }
}

void miniumount() {
    if (disk_fd != -1) {
        close(disk_fd);
        disk_fd = -1;
        printf("Disk unmounted successfully.\n");
    } else {
        printf("No disk is currently mounted.\n");
    }
}

void showsuper() {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    unsigned char buffer[BLOCK_SIZE];
    lseek(disk_fd, SUPERBLOCK_OFFSET, SEEK_SET);
    if (read(disk_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Failed to read superblock");
        return;
    }

    // Parse superblock information
    printf("Superblock information:\n");
    printf("  Number of inodes:       %u\n", *(unsigned short *)(buffer + 0));
    printf("  Number of zones:        %u\n", *(unsigned short *)(buffer + 2));
    printf("  Number of imap_blocks:  %u\n", *(unsigned short *)(buffer + 4));
    printf("  Number of zmap_blocks:  %u\n", *(unsigned short *)(buffer + 6));
    printf("  First data zone:        %u\n", *(unsigned short *)(buffer + 8));
    printf("  Log zone size:          %u\n", *(unsigned short *)(buffer + 10));
    printf("  Max size:               %u\n", *(unsigned int *)(buffer + 12));
    printf("  Magic:                  %u\n", *(unsigned short *)(buffer + 16));
    printf("  State:                  %u\n", *(unsigned short *)(buffer + 18));
    printf("  Zones:                  %u\n", *(unsigned short *)(buffer + 20));
}

void traverse(int long_list) {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    // Root directory reading logic
    lseek(disk_fd, ROOT_DIR_START, SEEK_SET); // Adjust ROOT_DIR_START based on your filesystem
    struct dir_entry {
        uint32_t inode; // Inode number
        char name[60];  // Name of the entry
    } dir_entry;

    printf("Directory Contents:\n");
    while (read(disk_fd, &dir_entry, sizeof(dir_entry)) > 0) {
        if (dir_entry.inode == 0) continue; // Skip unused entries

        if (long_list) {
            // Fetch inode details for long listing
            struct inode entry_inode;
            lseek(disk_fd, get_inode_offset(dir_entry.inode), SEEK_SET);
            read(disk_fd, &entry_inode, sizeof(entry_inode));

            char permissions[11];
            parse_permissions(entry_inode.mode, permissions);
            printf("%s %d %ld %s\n", permissions, entry_inode.uid, entry_inode.size, dir_entry.name);
        } else {
            printf("%s\n", dir_entry.name);
        }
    }
}

void showzone(int zone_number) {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    unsigned char buffer[BLOCK_SIZE];
    read_block(zone_number, buffer);

    printf("Zone %d content:\n", zone_number);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (i % 16 == 0) printf("\n%04x  ", i);
        printf("%c ", isprint(buffer[i]) ? buffer[i] : '.');
    }
    printf("\n");
}

void showfile(const char *filename) {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    // Locate the file in the root directory
    lseek(disk_fd, ROOT_DIR_START, SEEK_SET);
    struct dir_entry {
        uint32_t inode; // Inode number
        char name[60];  // Name of the entry
    } dir_entry;

    int found = 0;
    while (read(disk_fd, &dir_entry, sizeof(dir_entry)) > 0) {
        if (strcmp(dir_entry.name, filename) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("File '%s' not found in the root directory.\n", filename);
        return;
    }

    // Fetch file's inode
    struct inode file_inode;
    lseek(disk_fd, get_inode_offset(dir_entry.inode), SEEK_SET);
    read(disk_fd, &file_inode, sizeof(file_inode));

    // Read file content
    char buffer[BLOCK_SIZE];
    printf("Contents of file '%s':\n", filename);
    for (int i = 0; i < file_inode.size / BLOCK_SIZE + 1; i++) {
        lseek(disk_fd, get_data_block_offset(file_inode.blocks[i]), SEEK_SET);
        read(disk_fd, buffer, BLOCK_SIZE);
        for (int j = 0; j < BLOCK_SIZE && (i * BLOCK_SIZE + j) < file_inode.size; j++) {
            putchar(buffer[j]); // Print character to console
        }
    }
    printf("\n");
}

void read_block(int block_num, void *buffer) {
    off_t offset = block_num * BLOCK_SIZE;
    lseek(disk_fd, offset, SEEK_SET);
    if (read(disk_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Failed to read block");
    }
}

void parse_permissions(mode_t mode, char *perm) {
    perm[0] = (mode & S_IFDIR) ? 'd' : '-';
    perm[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm[3] = (mode & S_IXUSR) ? 'x' : '-';
    perm[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm[6] = (mode & S_IXGRP) ? 'x' : '-';
    perm[7] = (mode & S_IROTH) ? 'r' : '-';
    perm[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm[9] = (mode & S_IXOTH) ? 'x' : '-';
    perm[10] = '\0';
}
off_t get_inode_offset(int inode_number) {
    return INODE_TABLE_START + (inode_number - 1) * sizeof(struct inode); // Adjust INODE_TABLE_START
}

off_t get_data_block_offset(int block_number) {
    return DATA_BLOCK_START + block_number * BLOCK_SIZE; // Adjust DATA_BLOCK_START
}
