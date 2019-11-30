main:
	clear
	gcc diskinfo.c -o diskinfo -Wall
	gcc disklist.c -o disklist -Wall
	gcc diskget.c -o diskget -Wall
	gcc diskput.c -o diskput -Wall
	./diskinfo test.img
	./disklist test.img /
	./diskget test.img foo.txt foo.txt
	./diskget test.img mkfile.cc mkfile.cc
	./diskput test.img test.c test.c
	./diskput test.img onebyte.txt onebyte.txt
	./diskput test.img twobytes.txt twobytes.txt	
	./disklist test.img /


dev:
	clear
	gcc test.c -o test -Wall
	./test

resetImg:
	clear
	cp ../test.img ./test.img

