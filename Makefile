.phony all:
all: disks

disks:

	gcc -Wall diskinfo.c -o diskinfo
	gcc -Wall disklist.c -o disklist
	gcc -Wall diskget.c -o diskget
	gcc -Wall diskput.c -o diskput
	
.Phony clean:
clean:
	-rm -rf *.o *.exe