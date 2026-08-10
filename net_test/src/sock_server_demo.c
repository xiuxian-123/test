#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/my_socket.sock"
#include <log.h>

int main()
{
    struct sockaddr_un sock_server;
    memset(&sock_server, 0, sizeof(sock_server));
    sock_server.sun_family = AF_UNIX;
    strcpy(sock_server.sun_path, SOCK_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("socket failed\n");
        return -1;
    }
    unlink(SOCK_PATH);

    int res = bind(server_fd, (struct sockaddr *)&sock_server, sizeof(sock_server));
    if (res != 0) {
        log_error("bind failed\n");
        goto sock_server_close;
 
    }

    res =  listen(server_fd, 1024);
    if (res != 0) {
        log_error("listen failed\n");
        goto sock_server_close;
    }
    
    log_info("Server is listening on:%s\n", SOCK_PATH);

    int sock_client= accept(server_fd, NULL, NULL);
    if (sock_client < 0) {
        log_error("accept failed\n");
        goto sock_server_close;
    }
    
    char clientbuffer[1024] = {0};
    int read_data = read(sock_client, &clientbuffer, sizeof(clientbuffer));
    if (read_data < 0) {
        log_error("read failed\n");
        goto sock_client_close;
    }
    log_info("Received from client: %s\n", clientbuffer);
    
    close(server_fd);
    close(sock_client);
    return 0;
    
sock_server_close:
    close(server_fd);
sock_client_close:
    close(sock_client);
}

