#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <log.h>

#define COUNT 127

static int turn = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static char g_buffer[258] = { 0 };

void *producer(void *arg)
{
    for (int i = 1; i <= COUNT; i++) {
        pthread_mutex_lock(&g_mutex);
        while (turn != 0) {
            pthread_cond_wait(&g_cond, &g_mutex);
        }
        if (g_buffer[0] == 127) {
            g_buffer[0] = 0;
        } else {
            g_buffer[0]++;
        }

        memset(&g_buffer[2], 0, 256);
        fprintf(stdout, "producer: %d\n", g_buffer[0]);
        sprintf(&g_buffer[2], "message: %d", g_buffer[0]);
        
        turn = 1;
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_mutex);

    }

    return 0;
}

void *consumer(void *arg)
{
    for(int i = 1; i <= COUNT; i++) {
        pthread_mutex_lock(&g_mutex);
        while (turn != 1) {
            pthread_cond_wait(&g_cond, &g_mutex);
        }

        log_info("%s\n", g_buffer + 2);
        memset(&g_buffer[2], 0, 256);
        g_buffer[1]++;

        turn = 0;
        pthread_cond_signal(&g_cond);
        pthread_mutex_unlock(&g_mutex);

    }

    return 0;
}

int main()
{
    pthread_t pro_printf;
    pthread_t con_printf;

    memset(g_buffer, 0, sizeof(g_buffer));

    int rc = pthread_create(&pro_printf, NULL, producer, NULL);
    if (rc != 0) {
        log_error("Create failed\n");
    }

    rc = pthread_create(&con_printf, NULL, consumer, NULL);
    if (rc != 0) {
        log_error("Create failed\n");
    }

    pthread_join(pro_printf, NULL);
    pthread_join(con_printf, NULL);
    
    return 0;
}

