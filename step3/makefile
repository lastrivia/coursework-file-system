HEADERS := $(shell find ../src -name "*.h")

all: client disk fs

client: client.cpp $(HEADERS)
	g++ client.cpp -o client -std=c++20
disk: disk.cpp $(HEADERS)
	g++ disk.cpp -o disk -std=c++20
fs: fs.cpp $(HEADERS)
	g++ fs.cpp -o fs -std=c++20
clean:
	find . -type f ! -name '*.cpp' ! -name 'makefile' -delete