CXXFLAGS := -g -Wall -Werror -std=c++11
LDFLAGS := -lnsl

all: socket_example

socket_example: socket_example.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o *~ socket_example
