minix_disk_program.out: imagefile.img minix_disk_program.c
	gcc -o minix_disk_program minix_disk_program.c
	
	./minix_disk_program
