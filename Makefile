CFLAGS = -Wall -g -Werror -Wno-error=unused-variable -I.

all: server subscriber

server: server.cpp utils-common-func.hpp
	g++ $(CFLAGS) -o server server.cpp

subscriber: subscriber.cpp utils-common-func.hpp
	g++ $(CFLAGS) -o subscriber subscriber.cpp

.PHONY: clean run_server run_subscriber

run_server:
	./server

run_subscriber:
	./subscriber

clean:
	rm -rf server subscriber

