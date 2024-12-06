This project, titled Minix Disk Program, was developed as part of the CIS345 course, and it aims to provide functionality for interacting with a Minix disk image in C. The program allows users to perform a variety of essential operations on a Minix file system, such as mounting and unmounting a disk image, displaying the superblock details, traversing the root directory, reading zone content, and showing the contents of files within the disk image. These operations are carried out using low-level system calls, including open(), read(), and lseek(), which directly interact with the disk image and allow for manual access to the underlying file system structures.

The project is a collaborative effort, with 

Maksym Yarosh (ID: 2881321) - 50% 
Joseph Spitznagel (ID: 2863192) - 50%, 

For ease of use, a makefile is provided, allowing users to simply type make in the terminal to compile the program, resulting in the executable file minix_disk_program.

Included in the project submission is the disk image file imagefile.img. Users are expected to specify this file in the [file] field of the minimount function to load the Minix file system for further operations.

However, it is important to note that, as of the time of submission, the traverse function does not work properly. There are several potential reasons for this issue, including but not limited to:

An improper i-node table offset
An incorrect mode for the root i-node
Issues with reading the superblock correctly
While the possibility of a corrupted disk image has been considered, it has not been definitively ruled out. As a result of these difficulties, time constraints prevented the implementation of the bonus showfile function, which would have allowed users to view the contents of individual files.

In summary, the Minix Disk Program provides a fundamental set of tools for interacting with a Minix file system, though there are some issues with certain functions that are still being addressed. Future development will aim to resolve these issues and further enhance the program's functionality.

This version clarifies the project’s goals and the current status of specific features while maintaining the original information. It explains the functionality of the program, addresses known issues, and presents the project in a more comprehensive way.
