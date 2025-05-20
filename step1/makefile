HEADERS := $(shell find ../src -name "*.h")

client: client.cpp $(HEADERS)
	g++ client.cpp -o client -std=c++20
disk: disk.cpp $(HEADERS)
	g++ disk.cpp -o disk -std=c++20
clean:
	rm -f client *.raw