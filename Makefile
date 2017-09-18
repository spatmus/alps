appname := alPi

CXX := g++
CXXFLAGS := -std=c++11 -I. -lportaudio -lpthread -lsndfile
CFLAGS := -I. -DHAVE_CONFIG_H -DSO

srcfiles := $(shell find . -name "*.c*")
objects  := $(patsubst %.c, %.o, $(srcfiles))

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

#depend: .depend

#.depend: $(srcfiles)
#	rm -f ./.depend
#	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(shell find . -name "*.o")

#dist-clean: clean
#	rm -f *~ .depend

#include .depend
