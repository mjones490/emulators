PROJECTS = shell 6502_cpu 8080_cpu markII dynosaur
CLEAN_PROJECTS = $(addsuffix clean, $(PROJECTS))
BUILD_PROJECTS = $(addsuffix build, $(PROJECTS))

all : $(BUILD_PROJECTS)

.SECONDARY :

clean : $(CLEAN_PROJECTS)

%clean : %
	$(MAKE) -C $^ clean

%build : %
	$(MAKE) -C $^


