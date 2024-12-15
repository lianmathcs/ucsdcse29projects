all: chat-server

chat-server:	
		gcc -std=c11 -g -fsanitize=address chat-server.c http-server.c -o chat-server

clean:
	rm -f chat-server