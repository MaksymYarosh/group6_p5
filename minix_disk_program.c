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

    // Implementation omitted due to complexity.
    printf("Traverse functionality not fully implemented yet.\n");

    /*
    code should display the following for each file in this order (if applicable):
    
    values below are mode fields in i-node struct:
    S_IFREG    0100000   regular file = -_________
    S_IFDIR    0040000   directory = d_________
    S_IRWXU    00700     mask for file owner permissions = _rwx______
    S_IRUSR    00400     owner has read permission = _r________
    S_IWUSR    00200     owner has write permission = __w_______
    S_IXUSR    00100     owner has execute permission = ___x______
    S_IRWXG    00070     mask for group permissions = ____rwx___
    S_IRGRP    00040     group has read permission = ____r_____
    S_IWGRP    00020     group has write permission = _____w____
    S_IXGRP    00010     group has execute permission = ______x___
    S_IRWXO    00007     mask for permissions for others (not in group) = _______rwx
    S_IROTH    00004     others have read permission = _______r__
    S_IWOTH    00002     others have write permission = ________w_
    S_IXOTH    00001     others have execute permission = _________x
    
    number after permissions is number of links (? not sure, will ask the professor)
    number after # of links(?) is size in kilobytes(? not sure what size is default, will ask the professor)
    format of date last changed is month day year
    file name

    pseudocode for listing display in long format:
    if (strcmp(cmd, "-l") == 1) {
        for (int i = 0; i < numberOfFilesOnMinixDisk; i++) {
            if (fileType = normalFileValue(S_IFREG or 0100000)) {
                print("-");
            } else if (fileType = directoryValue(SIFDIR or 0040000)) {
                print("d");
            } else {
                print(fileTypeError);
            }
            
            if (ownerPermissionMask = S_IRWXU or 00700) {
                print("rwx");
            }
            
            if (ownerReadPermission = S_IRUSR or 00400) {
                print("r");
            } else {
                print("-");
            }
            
            if (ownerWritePermission = S_IWUSR or 00200) {
                print("w");
            } else {
                print("-");
            }
    
            if (ownerExecutePermission = S_IXUSR or 00100) {
                print("x");
            } else {
                print("-");
            }
            
            if (groupPermissionMask = S_IRWXG or 00070) {
                print("rwx");
            }
            
            if (groupReadPermission = S_IRGRP or 00040) {
                print("r");
            } else {
                print("-");
            }
            
            if (groupWritePermission = S_IWGRP or 00020) {
                print("w");
            } else {
                print("-");
            }
            
            if (groupExecutePermission = S_IXGRP or 00010) {
                print("x");
            } else {
                print("-");
            }
        
            if (otherPermissionMask = S_IRWXO or 00007) {
                print("rwx");
            }
        
            if (otherReadPermission = S_IROTH or 00004) {
                print("r");
            } else {
                print("-");
            }
            
            if (otherWritePermission = S_IWOTH or 00002) {
                print("w");
            } else {
                print("-");
            }
            
            if (otherExecutePermission = S_IXOTH or 00001) {
                print("x");
            } else {
                print("-");
            }
            
            links (?), file size, date last modifed code goes here
            
            print(fileName);
            print(\n);
        }
    }
    */
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

    // Implementation omitted due to complexity.
    printf("Showfile functionality not fully implemented yet.\n");
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
