# makefile template

include MakeInclude

LDLIBS =  

VLAN_OBJS = vconfig.o

ALL_OBJS = ${VLAN_OBJS}

VCONFIG = vconfig                  #program to be created


all: ${VCONFIG}


#This is pretty silly..
vconfig.h: Makefile
	touch vconfig.h


${VCONFIG}: $(VLAN_OBJS)
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o $(VCONFIG) $(VLAN_OBJS) $(LDLIBS)


$(ALL_OBJS): %.o: %.cc %.h
	@echo " "
	@echo "Making $<"
	$(CC) $(CCFLAGS) \
	-c $<

clean:
	rm -f *.o

purge: clean
	rm -f *.flc ${VCONFIG} vconfig.h
	rm -f *~




