main:
	clear
	gcc diskinfo.c -o diskinfo -Wall
	gcc disklist.c -o disklist -Wall
	gcc diskget.c -o diskget -Wall
	# ./diskinfo test.img
	# ./disklist test.img mkfile.cc
	# ./disklist test.img /
	# ./diskget test.img foo.txt foo.txt
	./diskget test.img mkfile.cc mkfile.cc

dev:
	clear
	gcc test.c -o test -Wall
	./test