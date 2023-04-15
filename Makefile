build-client:
	g++ client.cpp -o ipkcpc

build-server:
	g++ -g server.cpp -o ipkcpd

clean:
	rm ipkcpc
