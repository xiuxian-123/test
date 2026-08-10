#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>

#include <log.h>

#define MESSAGE "Hello world"

static struct sockaddr_in sock_addr;

void *tcp_server(void *arg) 
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("socket failed:%d\n", errno);
        goto server_close;
    }

    int res_bind = bind(server_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res_bind != 0) {
        log_error("bind failed:%d\n", errno);
        goto server_close;
    }

    int res_listen = listen(server_fd, 1024);
    if (res_listen != 0) {
        log_error("listen failed:%d\n", errno);
        goto server_close;
    }

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        log_error("client failed:%d\n", errno);
        goto client_close;
    }

    char recv_buffer[1024] = {0};

    int res_recv = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
    if (res_recv < 0) {
        log_error("recv failed:%d\n", errno);
        goto client_close;
    }
    
    log_info("%s", recv_buffer);
    
    close(server_fd);
    close(client_fd);
    return NULL;

server_close:
    close(server_fd);
    return NULL;
client_close:
    close(client_fd);
    return NULL;
}

void *tcp_client(void *arg)
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        log_error("socket failed:%d\n", errno);
        goto client_close;
    }

    int res_connect = connect(client_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res_connect != 0) {
        log_error("connect failed:%d\n", errno);
        goto client_close;
    }

    int res_send = send(client_fd, MESSAGE, sizeof(MESSAGE), 0);
    if (res_send < 0) {
        log_error("send failed:%d\n", errno);
        goto client_close;
    }

    close(client_fd);
    return NULL;

client_close:
    close(client_fd);
    return NULL;
}
int main()
{
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family      = AF_INET;
    sock_addr.sin_port        = htons(10000);
    sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    pthread_t server = 0;
    pthread_t client = 0;

    pthread_create(&server, NULL, tcp_server, NULL);
    pthread_create(&client, NULL, tcp_client, NULL);

    pthread_join(server, 0);
    pthread_join(client, 0);

    return 0;
}

