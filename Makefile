CPP=g++
CPPFLAGS=-Wall -Wextra -Werror -O1 -g -std=c++17 -pthread

TARGETS=simple-server

all: $(TARGETS)

simple-server:	simple-server.cpp bounded_buffer.cpp
	$(CPP) $^ -o $@ $(CPPFLAGS)

clean:
	rm -f $(TARGETS)	

