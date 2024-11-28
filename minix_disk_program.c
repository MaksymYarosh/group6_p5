#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>  // Fixed-width integer types like uint16_t, uint32_t

// Constants
#define BLOCK_SIZE 1024
#define SUPERBLOCK_OFFSET 1024
#define ROOT_DIR_START 4096        // Adjust based on your Minix filesystem
#define INODE_TABLE_START 8192     // Adjust based on your Minix filesystem
#define DATA_BLOCK_START 16384     // Adjust based on your Minix filesystem
#define MINIX_MAGIC 0x137F

// Global variables
int disk_fd = -1; // File descriptor for the mounted Minix disk

// Structures
struct inode {
    mode_t mode;          // File mode
    uint16_t uid;         // User ID
    uint32_t size;        // File size
    time_t atime;         // Last access time
    time_t mtime;         // Last modification time
    time_t ctime;         // Creation time
    uint32_t blocks[10];  // Direct and indirect block pointers
};

struct dir_entry {
    uint32_t inode;       // Inode number
    char name[60];        // Name of the entry
};

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
off_t get_inode_offset(int inode_number);
off_t get_data_block_offset(int block_number);

// Main function
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

// Function definitions

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

    // Validate magic number
    if (*(unsigned short *)(buffer + 16) != MINIX_MAGIC) {
        printf("Invalid Minix file system (magic number mismatch).\n");
        return;
    }

    // Parse and display superblock information
    printf("Superblock information:\n");
    printf("  Number of inodes:       %u\n", *(uint16_t *)(buffer + 0));
    printf("  Number of zones:        %u\n", *(uint16_t *)(buffer + 2));
    printf("  Number of imap_blocks:  %u\n", *(uint16_t *)(buffer + 4));
    printf("  Number of zmap_blocks:  %u\n", *(uint16_t *)(buffer + 6));
    printf("  First data zone:        %u\n", *(uint16_t *)(buffer + 8));
    printf("  Log zone size:          %u\n", *(uint16_t *)(buffer + 10));
    printf("  Max size:               %u\n", *(uint32_t *)(buffer + 12));
    printf("  Magic:                  %u\n", *(uint16_t *)(buffer + 16));
    printf("  State:                  %u\n", *(uint16_t *)(buffer + 18));
    printf("  Zones:                  %u\n", *(uint16_t *)(buffer + 20));
}

void traverse(int long_list) {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    lseek(disk_fd, ROOT_DIR_START, SEEK_SET);
    struct dir_entry dir_entry;

    printf("Directory Contents:\n");
    while (read(disk_fd, &dir_entry, sizeof(dir_entry)) > 0) {
        if (dir_entry.inode == 0) continue; // Skip unused entries

        if (long_list) {
            struct inode entry_inode;
            lseek(disk_fd, get_inode_offset(dir_entry.inode), SEEK_SET);
            read(disk_fd, &entry_inode, sizeof(entry_inode));

            char permissions[11];
            parse_permissions(entry_inode.mode, permissions);
            printf("%s %hu %u %s\n", permissions, entry_inode.uid, entry_inode.size, dir_entry.name);
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
    off_t offset = get_data_block_offset(zone_number);

    lseek(disk_fd, offset, SEEK_SET);
    if (read(disk_fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Failed to read zone");
        return;
    }

    printf("Contents of zone %d:\n", zone_number);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (isprint(buffer[i])) {
            printf("%c", buffer[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

void showfile(const char *filename) {
    if (disk_fd == -1) {
        printf("No disk is currently mounted.\n");
        return;
    }

    // Traverse root directory to find the file
    lseek(disk_fd, ROOT_DIR_START, SEEK_SET);
    struct dir_entry dir_entry;

    while (read(disk_fd, &dir_entry, sizeof(dir_entry)) > 0) {
        if (dir_entry.inode == 0) continue;

        if (strcmp(dir_entry.name, filename) == 0) {
            struct inode file_inode;
            lseek(disk_fd, get_inode_offset(dir_entry.inode), SEEK_SET);
            read(disk_fd, &file_inode, sizeof(file_inode));

            printf("Contents of file '%s':\n", filename);
            for (int i = 0; i < 10 && file_inode.blocks[i] != 0; i++) {
                showzone(file_inode.blocks[i]);
            }
            return;
        }
    }

    printf("File '%s' not found in root directory.\n", filename);
}

off_t get_inode_offset(int inode_number) {
    return INODE_TABLE_START + (inode_number - 1) * sizeof(struct inode);
}

off_t get_data_block_offset(int block_number) {
    return DATA_BLOCK_START + block_number * BLOCK_SIZE;
}

void parse_permissions(mode_t mode, char *perm) {
    strcpy(perm, "----------");

    if (S_ISDIR(mode)) perm[0] = 'd'; // Directory
    if (S_ISCHR(mode)) perm[0] = 'c'; // Character device
    if (S_ISBLK(mode)) perm[0] = 'b'; // Block device
    if (S_ISLNK(mode)) perm[0] = 'l'; // Symlink

    if (mode & S_IRUSR) perm[1] = 'r'; // Owner read
    if (mode & S_IWUSR) perm[2] = 'w'; // Owner write
    if (mode & S_IXUSR) perm[3] = 'x'; // Owner execute

    if (mode & S_IRGRP) perm[4] = 'r'; // Group read
    if (mode & S_IWGRP) perm[5] = 'w'; // Group write
    if (mode & S_IXGRP) perm[6] = 'x'; // Group execute

    if (mode & S_IROTH) perm[7] = 'r'; // Others read
    if (mode & S_IWOTH) perm[8] = 'w'; // Others write
    if (mode & S_IXOTH) perm[9] = 'x'; // Others execute
}
