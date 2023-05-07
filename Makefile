.PHONY : all
all:
	export LD_LIBRARY_PATH=.
	make -f Makefile.libmfs
	make -f Makefile.server

.PHONY : clean
clean:
	make clean -f Makefile.libmfs
	make clean -f Makefile.server
