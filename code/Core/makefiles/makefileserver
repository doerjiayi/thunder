CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
ROOT = .
CMDSUBDIRS=$(shell find $(ROOT) -name 'Cmd*' -type d)
MODULESUBDIRS=$(shell find $(ROOT) -name 'Module*' -type d)

.PHONY : subdirs  
  
subdirs: 
	@for dir in $(CMDSUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f Makefile  CFLAG='$(CFLAG)') ; done
	@for dir in $(MODULESUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f Makefile  CFLAG='$(CFLAG)') ; done
	
clean:  
	@for dir in $(CMDSUBDIRS); do echo "cleaning in $$dir"; ( cd $$dir && make clean -f Makefile CFLAG='$(CFLAG)') ; done
	@for dir in $(MODULESUBDIRS); do echo "cleaning in $$dir"; ( cd $$dir && make clean -f Makefile CFLAG='$(CFLAG)') ; done
	

