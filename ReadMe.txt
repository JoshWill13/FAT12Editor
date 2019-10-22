Josh Williams
CSC 360, Assignment 3

compile programs with: make
run programs with:

	./diskinfo 	(IMA file) 
	./disklist 	(IMA file)
	./diskget 	(IMA file) (file we want to extract)
	./diskput 	(IMA file) (file we want to insert and directory we want to place it)
		(ex: ./diskput my.IMA /dir1/dir2/sample.txt)

Programs that preform operations on FAT12 file systems used by MS-DOS. Code is slightly messy and contains lots of 
duplication that could be taken care of by implementing some helper classes.
		
diskinfo: Obtains info about the FAT12 IMA file.

disklist: Obtains info about files and directories as well as how they are layed out. 

diskget: Obtains a specified file from a .IMA file and writes it to its own file in the current linux directory.

diskput: Inserts a specified file into a FAT12 .IMA file in a specified directory (placed in root otherwise).