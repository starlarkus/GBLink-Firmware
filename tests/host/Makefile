# Host-side tests for Zephyr-free firmware code (no Zephyr SDK needed).
CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -O1 -g

all: run

awProtocolTest: awProtocolTest.cpp ../../src/sections/awProtocol.hpp
	$(CXX) $(CXXFLAGS) -o $@ awProtocolTest.cpp

run: awProtocolTest
	./awProtocolTest

clean:
	rm -f awProtocolTest

.PHONY: all run clean
