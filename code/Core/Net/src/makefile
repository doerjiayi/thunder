CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
ROOT = .
NET_MAKEFILES=$(shell find $(ROOT) -name 'makefile.*' -type f)

# make -f makefile.other -e unit_test=y

.PHONY : mks

mks: 
	cd storage/ && chmod +x ./gen_storage_proto.sh && ./gen_storage_proto.sh && cd ..
	cd protocol/ && chmod +x ./gen_sys_proto.sh && ./gen_sys_proto.sh && cd ..
	@for make_file in $(NET_MAKEFILES); do echo "making $$make_file"; make clean -f $$make_file; make -f $$make_file $(UNIT_TEST); done

clean:  
	@for make_file in $(NET_MAKEFILES); do echo "cleaning in $$make_file"; make clean -f $$make_file; done
	rm -f libNet.so
