#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <curl/curl.h>
#include <errno.h>

#include <log.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *server_thread(void *arg)
{
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port   = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("socket failed:%d\n", errno);
        goto server_close;
    }

    int res_bind = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (res_bind != 0) {
        log_error("bind failed:%d\n", errno);
        goto server_close;
    }

    int res_listen = listen(server_fd, 2);
    if (res_listen != 0) {
        log_error("listen failed:%d\n", errno);
        goto server_close;
    }
    log_info("[Server Thread] listening\n");

    socklen_t addrlen = sizeof(address);

    int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0) {
        log_error("accept failed:%d\n", errno);
        goto client_close;
    }

    char buffer [BUFFER_SIZE] = {0};

    int res_read = read(client_fd, buffer, BUFFER_SIZE);
    if (res_read < 0) {
        log_error("read failed:%d\n", errno);
        goto client_close;
    }

    log_info("Received Request:\n%s\n", buffer);

    const char *html_body = "<h1>Hell World</h1>";
    char response[BUFFER_SIZE] = {0};
    int resp_len = snprintf(response, sizeof(response), 
                            "HTTP/1.1 200 OK\r\n" 
                            "Content-Type: text/html\r\n" 
                            "Content-Length: %lu\r\n" 
                            "\r\n" 
                            "%s", strlen(html_body), html_body);

    write(client_fd, response, resp_len);

    log_info("Server thread exited.\n");
    
    close(client_fd);
    close(server_fd);
    return NULL;

server_close:
    close(server_fd);
    return NULL;
client_close:
    close(client_fd);
    return NULL;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    printf("[Client Thread] Received from server: %.*s\n", (int)total_size, ptr);
    return total_size;
}

void *client_thread(void *arg) 
{
    sleep(1);

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        log_error("curl handle failde:%d\n", errno);
        return NULL;
    }
    
    CURLcode res;

    res = curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080");
    if (res != CURLE_OK) {
        log_error("curl_easu_init failed\n");
        goto curl_close;
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    if (res != CURLE_OK) {
        log_error("setopt WRITEFUNTION failed:%d\n", errno);
        goto curl_close;
    }

    log_info("Sending GET request to localhost:%d...\n", PORT);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("request failed\n");
        goto curl_close;
    }

    log_info("Request completed successfully.\n");

    return NULL;

curl_close:
    curl_easy_cleanup(curl);
}

int main()
{
    pthread_t server = 0;
    pthread_t client = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    pthread_create(&server, NULL, server_thread, NULL);
    pthread_create(&client, NULL, client_thread, NULL);

    pthread_join(server, NULL);
    pthread_join(client, NULL);
    
    curl_global_cleanup();
    
    return 0;
}

