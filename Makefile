CXX = g++
CXXFLAGS = -W -Wall -O2 -g

all: test

test: test.o bencode.o
	$(CXX) -o $@ test.o bencode.o
