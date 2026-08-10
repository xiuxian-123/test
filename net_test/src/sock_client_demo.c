#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SOCK_PATH "/tmp/my_socket.sock"
#define MESSAGE "Hellow Wrold"

#include <log.h>

int main()
{
    struct sockaddr_un sock_client;
    memset(&sock_client, 0, sizeof(sock_client));
    sock_client.sun_family = AF_UNIX;
    strcpy(sock_client.sun_path, SOCK_PATH);

    int clientfb = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientfb == -1) {
        log_error("socket failed:%d\n", errno);
        goto sock_client_close;
    }
    
    int res = connect(clientfb, (struct sockaddr *)&sock_client, sizeof(sock_client));
    if (res != 0) {
        log_error("connect faild:%d\n", errno);
        goto sock_client_close;
    }

    char writebuffer[1024] = {0};
    int writefd = write(clientfb, MESSAGE, sizeof(MESSAGE));
    if (writefd < 0) {
        log_error("write failed:%d\n", errno);
        goto sock_client_close;
    }

    close(clientfb);
    return 0;

sock_client_close:
    close(clientfb);
}

