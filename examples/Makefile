#
# 'make'        build executable file
# 'make clean'  removes all .o and executable files
#

# dependency directory
DEPSDIR := $(shell mkdir -p .deps; echo .deps)

# get the shell name to determine the OS
UNAME := $(shell uname)

# define the C compiler to use
CC := gcc
ifeq ($(UNAME), Linux)
CXX := g++-4.7 -std=gnu++0x
endif
ifeq ($(UNAME), Darwin)
CXX := $(shell for i in 4.7 4.6 4.5; do if g++-mp-$$i -v >/dev/null 2>&1; then echo g++-mp-$$i; exit; fi; done; echo false) -std=gnu++0x
OBJC := gcc
endif
LINK := $(CXX)

DEBUG = 1

# define any compile-time flags
CFLAGS := -fopenmp -funroll-loops -fforce-addr -O3 -W -Wall -Wextra #-ffast-math
ifeq ($(DEBUG),1)
CFLAGS += -g -fno-inline
endif
ifeq ($(PROFILE),1)
CFLAGS += -g -pg
endif
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP

# Other in-code flags
CFLAGS +=

# define any directories containing header files other than /usr/include
#   include directories like -Ipath/to/files
INCLUDES = -I. -I../include -I../include/tree -I../include/executor -I../kernel -I./BEM

# define any libraries to link into executable
#   To link in libraries (libXXX.so or libXXX.a) use -lXXX options
LIBS +=

##################
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
##################

# 'make' - default rule
all: LaplaceBEM StokesBEM

LaplaceBEM: LaplaceBEM.o
	$(LINK) $(CFLAGS) $(LDFLAGS) -o $@ $^

LaplaceBEM_time: LaplaceBEM_time.o
	$(LINK) $(CFLAGS) $(LDFLAGS) -o $@ $^

StokesBEM: StokesBEM.o
	$(LINK) $(CFLAGS) $(LDFLAGS) -o $@ $^

reader_test: reader_test.o
	$(LINK) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lGL -lGLU -lglut

# suffix replacement rule for building .o's from .cpp's
#   $<: the name of the prereq of the rule (a .cpp file)
#   $@: the name of the target of the rule (a .o file)
.cpp.o:
	$(CXX) $(CFLAGS) $(DEPCFLAGS) $(INCLUDES) -c -o $@ $<

# 'make clean' - deletes all .o and temp files, exec, and dependency file
clean:
	-$(RM) *.o *~ */*~
	-$(RM) LaplaceBEM YukawaBEM StokesBEM
	-$(RM) *.charge *.face *.vert
	$(RM) -r $(DEPSDIR)

DEPFILES := $(wildcard $(DEPSDIR)/*.d) $(wildcard $(DEPSDIR)/*/*.d)
ifneq ($(DEPFILES),)
-include $(DEPFILES)
endif

# define rules that do not actually generate the corresponding file
.PHONY: clean all
