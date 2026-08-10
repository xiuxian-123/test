CC=gcc
OUTPUT=./out
SRC=./src
MKDIR=mkdir

bulid:
	$(MKDIR) $(OUTPUT) -p
	$(CC) $(SRC)/sock_server_demo.c -o $(OUTPUT)/sock_server_demo -I./include
	$(CC) $(SRC)/sock_client_demo.c -o $(OUTPUT)/sock_client_demo -I./include
	$(CC) $(SRC)/tcp_socket_demo.c -o $(OUTPUT)/tcp_socket_demo -I./include
	$(CC) $(SRC)/udp_socket_demo.c -o $(OUTPUT)/udp_socket_demo -I./include
	$(CC) $(SRC)/http_socket_demo.c -o $(OUTPUT)/http_socket_demo -I./include -lcurl
remove:
	rm $(OUTPUT)/sock_server_demo
	rm $(OUTPUT)/sock_cilent_demo
	rm $(OUTPUT)/tcp_socket_demo
	rm $(OUTPUT)/udp_socket_demo
	rm $(OUTPUT)/http_socket_demo
