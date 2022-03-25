all: adytum-server.cpp server.cpp server.h adytum-client.cpp client.cpp client.h global.h
	g++ -o adytum-client adytum-client.cpp client.cpp -lpthread -g
	g++ -o adytum-server adytum-server.cpp server.cpp -lpthread -lmysqlclient -lhiredis -g
test_server: adytum-server.cpp server.cpp server.h global.h
	g++ -o adytum-server adytum-server.cpp server.cpp -lpthread -lmysqlclient -lhiredis -g
test_client: adytum-client.cpp client.cpp client.h global.h
	g++ -o adytum-client adytum-client.cpp client.cpp -lpthread -g
clean:
	rm adytum-server
	rm adytum-client
