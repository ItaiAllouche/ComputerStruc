# 046267 Computer Architecture - Winter 20/21 - HW #2
CXX = g++
CLINK = $(CXX)
CXXFLAGS = -g -Wall -std=c++11

cacheSim: cacheSim.o cache.o
	$(CLINK) $(CXXFLAGS) -o cacheSim cacheSim.o cache.o 

cacheSim.o: cacheSim.cpp
	$(CXX) $(CXXFLAGS) -c cacheSim.cpp

cache.o: cache.cpp cache.h
	$(CXX) $(CXXFLAGS) -c cache.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
