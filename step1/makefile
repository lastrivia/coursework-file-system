HEADERS := $(shell find ../src -name "*.h")

all: client disk

client: client.cpp $(HEADERS)
	g++ client.cpp -o client -std=c++20 -lreadline
disk: disk.cpp $(HEADERS)
	g++ disk.cpp -o disk -std=c++20
clean:
	find . -type f ! -name '*.cpp' ! -name 'makefile' -delete