# group6_p5
# CIS345_Project5

Maksym Yarosh 2881321 - 50%
Joseph Spitznagel 2863192 - 50%

This project implements a Minix Disk Program in C. It provides functionality to interact with a Minix disk image, allowing users to perform operations such as mounting, unmounting, displaying superblock details, traversing the root directory, reading zone content, and showing file contents. The program uses low-level system calls (open(), read(), lseek(), etc.) to access the disk image directly.

To compile the program, use the following command:

gcc -o minix_disk_program minix_disk_program.c

To run the program, use:

./minix_disk_program
