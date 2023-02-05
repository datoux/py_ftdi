MKDIR=mkdir
CP=cp
GREP=grep
CCC=g++

# Object Directory
OBJECTDIR = ./
TARGET    = py_ftdi.so

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/py_ftdi.o \

# C Compiler Flags
CFLAGS=-o $@ 
LDLIBSOPTIONS= -L /usr/local/Cellar/python3/3.5.1/Frameworks/Python.framework/Versions/3.5/lib -lpython3.5 -shared
INC = -I/usr/local/Cellar/python3/3.5.1/Frameworks/Python.framework/Versions/3.5/include/python3.5m \
-I/usr/local/lib/python3.5/site-packages/numpy/core/include/numpy

#LDLIBSOPTIONS= -L/System/Library/Frameworks/Python.framework/Versions/2.7/lib -lpython2.7 -shared
#INC = -I/System/Library/Frameworks/Python.framework/Versions/2.7/Include/python2.7 \
	  -I/System/Library/Frameworks/Python.framework/Versions/2.7/Extras/lib/python/numpy/core/include/numpy


all: $(TARGET)

clean:
	rm -f $(OBJECTDIR)/*.o  $(OBJECTDIR)/*.o.d $(TARGET)

$(OBJECTDIR): 
	@mkdir -p "$@";

$(TARGET): $(OBJECTDIR) $(OBJECTFILES)
	@$(CCC) -o $(TARGET) -fPIC $(OBJECTFILES) $(LDLIBSOPTIONS) 

$(OBJECTDIR)/%.o: %.cpp
	@$(CCC) $(INC) $(CFLAGS) -o $@ -c $<


DEPFILES=$(wildcard $(addsuffix .d, ${OBJECTFILES}))
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
