test_server: test_server.cpp server.cpp server.h
	g++ -o test_server test_server.cpp server.cpp -pthread -Wall
clean:
	rm test_server
