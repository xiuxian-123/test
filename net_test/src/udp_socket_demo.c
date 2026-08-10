#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <log.h>

#define RECV_MESSAGE "Hello World"
#define REPLYMESSAGE "already recved"

static struct sockaddr_in sock_addr;
static char buffer [1024] = {0};
static socklen_t addr_len = sizeof(sock_addr);
void *udp_server(void *arg)
{
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        log_error("socket failed:%d\n", errno);
        goto server_close;
    }
    int res_bind = bind(server_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res_bind != 0) {
        log_error("bind failed:%d\n", errno);
        goto server_close;
    }
    
    int res_recv = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sock_addr, &addr_len);
    if (res_recv < 0) {
        log_error("recv failed:%d\n", errno);
        goto server_close;
    }

    int res_send = sendto(server_fd, REPLYMESSAGE, sizeof(REPLYMESSAGE), 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res_send < 0) {
        log_error("send failed:%d\n", errno);
        goto server_close;
    }
    
    log_info("%s", buffer);

    close(server_fd);
    return NULL;

server_close:
    close(server_fd);
    return NULL;
}

void *udp_client(void *arg)
{
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        log_error("socket failed:%d\n", errno);
        goto client_close;
    }

    int res_send = sendto(client_fd, RECV_MESSAGE, sizeof(RECV_MESSAGE), 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (res_send < 0) {
        log_error("send failed:%d\n", errno);
        goto client_close;
    }

    int res_recv = recvfrom(client_fd, buffer, sizeof(buffer), 0, NULL, &addr_len);
    if (res_recv < 0) {
        log_error("recv failed:%d\n", errno);
        goto client_close;
    }
    
    log_info("%s", buffer);
    close(client_fd);
    return NULL;

client_close:
    close(client_fd);
    return NULL;
}
int main()
{
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port   = htons(10000);
    sock_addr.sin_addr.s_addr = INADDR_ANY;

    pthread_t server = 0;
    pthread_t client = 0;

    pthread_create(&server, NULL, udp_server, NULL);
    pthread_create(&client, NULL, udp_client, NULL);
    
    pthread_join(server, NULL);
    pthread_join(client, NULL);
    return 0;
}

