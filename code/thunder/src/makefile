CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
ROOT = .
STARTSHIP_MAKEFILES=$(shell find $(ROOT) -name 'makefile.*' -type f)

# make -f makefile.other -e unit_test=y

.PHONY : mks

mks: 
	@for make_file in $(STARTSHIP_MAKEFILES); do echo "making $$make_file"; make clean -f $$make_file; make -f $$make_file $(UNIT_TEST); done

clean:  
	@for make_file in $(STARTSHIP_MAKEFILES); do echo "cleaning in $$make_file"; make clean -f $$make_file; done

cleanall:
	@for make_file in $(STARTSHIP_MAKEFILES); do echo "cleaning in $$make_file"; make clean -f $$make_file; done
	test -f libthunder.so && rm -f libthunder.so
	test -f HelloThunder && rm -f HelloThunder
	rm ./*Server ./*Agent* 
