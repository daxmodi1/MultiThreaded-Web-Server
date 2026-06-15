CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -pthread
LDLIBS =

objects = myhttpd.o myserver.o senddata.o parse.o

myhttpd: $(objects)
	$(CXX) $(CXXFLAGS) -o myhttpd $(objects) $(LDLIBS)

myhttpd.o: myhttpd.cpp myhttpd.h myserver.h parse.h
	$(CXX) $(CXXFLAGS) -c myhttpd.cpp

myserver.o: myserver.cpp myserver.h myhttpd.h parse.h socket_handle.h
	$(CXX) $(CXXFLAGS) -c myserver.cpp

senddata.o: senddata.cpp senddata.h parse.h myhttpd.h socket_handle.h
	$(CXX) $(CXXFLAGS) -c senddata.cpp

parse.o: parse.cpp parse.h myhttpd.h senddata.h
	$(CXX) $(CXXFLAGS) -c parse.cpp

.PHONY: clean
clean:
	rm -f myhttpd $(objects)
