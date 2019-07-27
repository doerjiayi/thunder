CXX = g++
ROOT = .
CMDSUBDIRS=$(shell find $(ROOT) -name 'Cmd*' -type d)
MODULESUBDIRS=$(shell find $(ROOT) -name 'Module*' -type d)

.PHONY : subdirs  
  
subdirs: 
	@for dir in $(CMDSUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f Makefile  CFLAG='$(CFLAG)') || exit 1; done
	@for dir in $(MODULESUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f Makefile  CFLAG='$(CFLAG)') || exit 1; done
	
clean:  
	@for dir in $(CMDSUBDIRS); do echo "cleaning in $$dir"; ( cd $$dir && make clean -f Makefile) || exit 1; done
	@for dir in $(MODULESUBDIRS); do echo "making in $$dir"; ( cd $$dir && make -f Makefile CFLAG='$(CFLAG)') || exit 1; done
	

