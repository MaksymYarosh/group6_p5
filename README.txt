# group6_p5
# CIS345_Project5

Maksym Yarosh 2881321 - 50%
Joseph Spitznagel 2863192 - 50%

This project implements a Minix Disk Program in C. It provides functionality to interact with a Minix disk image, allowing users to perform operations such as mounting, unmounting, displaying superblock details, traversing the root directory, reading zone content, and showing file contents. The program uses low-level system calls (open(), read(), lseek(), etc.) to access the disk image directly.

A makefile has been included for the project.

just type "make" to create the file "minix_disk_program"

*: the file "imagefile.img" is included in this submission. Use this file in the [file] field of minimount.

**: at the time of submission, the traverse function does not work properly.
There are several possibilities for why this function is not working:
-improper i-node table offset
-incorrect mode for the root i-node
-incorrect reading of the superblock
at this time, the possibility of a corrupted image is also equally likely.

***: difficulties with the traverse function did not permit time to implement the bonus showfile function.
