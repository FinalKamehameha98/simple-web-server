CPP=g++
CPPFLAGS=-Wall -Wextra -Werror -O1 -g -std=c++17 -pthread

TARGETS=simple-server regex-test

all: $(TARGETS)

simple-server: simple-server.cpp
	$(CPP) $^ -o $@ $(CPPFLAGS)

regex-test: regex-test.cpp
	$(CPP) $^ -o $@ -Wall -Wextra -Werror -g -std=c++17

clean:
	rm -f $(TARGETS)	

