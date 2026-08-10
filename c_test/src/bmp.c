#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <log.h>

#define BM_MAGIC (0x4D42)

#pragma pack(push, 1)
struct BMP_file_header {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

/* typedef struct BMP_file_header { */
/*     uint16_t bfType; */
/*     uint32_t bfSize; */
/*     uint16_t bfReserved1; */
/*     uint16_t bfReserved2; */
/*     uint32_t bfOffBits; */
/* } BMP_file_header_s ; */

struct BMP_info_header {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int rowSize;
    BMP_file_header fileHeader;
    BMP_info_header infoHeader;
} BMPCONTEXT;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static int turn = 0;

读入bmp
资源初始化

初始化     input
running    thread  process
去初始化   output

static void *input_bmp(void *arg) 
{
    BMPCONTEXT *ctx = (BMPCONTEXT *)arg;
    const char *input_path = "input.bmp";
    int fd_in = 0;
    
    pthread_mutex_lock(&g_mutex);
    while (turn != 0) {
        pthread_cond_wait(&g_cond, &g_mutex);
    }

    fd_in = open(input_path, O_RDONLY);
    if (fd_in == -1) {
        log_error("open the file failed\n");
        goto error_unlock;
    }

    read(fd_in, &ctx->fileHeader, sizeof(BMP_file_header));
    read(fd_in, &ctx->infoHeader, sizeof(BMP_info_header));

    if (ctx->fileHeader.bfType != BM_MAGIC || ctx->infoHeader.biBitCount != 24) {
        log_error("just 24bit BMP\n");
        goto error_close_fd;
    }
    
    ctx->width   = ctx->infoHeader.biWidth;
    ctx->height  = abs(ctx->infoHeader.biHeight);
    ctx->rowSize = ((ctx->width * 3 + 3) / 4) * 4;
    ctx->pixels  = (uint8_t*)malloc(ctx->rowSize * ctx->height);
    if (ctx->pixels == 0) {
        log_error("malloc failed\n");
        goto error_close_fd;
    }
    memset(ctx->pixels, 0, ctx->rowSize * ctx->height);

    lseek(fd_in, ctx->fileHeader.bfOffBits, SEEK_SET);
    read(fd_in, ctx->pixels, ctx->rowSize * ctx->height);
    close(fd_in);

    turn = 1;
    pthread_mutex_unlock(&g_mutex);
    pthread_cond_signal(&g_cond);

    return NULL;

error_close_fd:
    close(fd_in);
error_unlock:
    return NULL;
}

static void *process_bmp(void *arg) 
{
    BMPCONTEXT *ctx = (BMPCONTEXT *)arg;
    pthread_mutex_lock(&g_mutex);
    while (turn != 1) {
        pthread_cond_wait(&g_cond, &g_mutex);
    }
    
    int block_size = 20;
    int height = ctx->height;
    int width = ctx->width;
    uint8_t *pixels = ctx->pixels;
    int rowSize = ctx->rowSize;

    for (int y = 0; y < height; y += block_size) {
        for (int x = 0; x < width; x += block_size) {
            int end_y = (y + block_size > height) ? height : (y + block_size);
            int end_x = (x + block_size > width) ? width : (x + block_size);

            int sum_b = 0;
            int sum_g = 0;
            int sum_r = 0;
            int count = 0;

            for (int by = y; by < end_y; by++) {
                for (int bx = x; bx < end_x; bx++) {
                    int index = by * rowSize + bx * 3;
                    sum_b += pixels[index];
                    sum_g += pixels[index + 1];
                    sum_r += pixels[index + 2];
                    count++;
                }
            }

            uint8_t avg_b = (uint8_t)(sum_b / count);
            uint8_t avg_g = (uint8_t)(sum_g / count);
            uint8_t avg_r = (uint8_t)(sum_r / count);

            for (int by = y; by < end_y; by++) {
                for (int bx = x; bx < end_x; bx++) {
                    int index = by * rowSize + bx * 3;
                    pixels[index] = avg_b;
                    pixels[index + 1] = avg_g;
                    pixels[index + 2] = avg_r;
                }
            }
        }
    }
    turn = 2;
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mutex);
}

static void *output_bmp(void *arg) 
{
    BMPCONTEXT *ctx = (BMPCONTEXT *)arg;
    const char *output_path = "output.bmp";
    
    pthread_mutex_lock(&g_mutex);
    while (turn != 2) {
        pthread_cond_wait(&g_cond, &g_mutex);
    }
    int fd_out = open(output_path, O_CREAT | O_WRONLY | O_TRUNC);
    if (fd_out == -1) {
        log_error("output failed\n");
        free(ctx->pixels);
        return NULL;
    }

    ctx->fileHeader.bfSize = sizeof(BMP_file_header) + sizeof(BMP_info_header) + (ctx->rowSize * ctx->height);
    ctx->infoHeader.biSizeImage = ctx->rowSize * ctx->height;

    write(fd_out, &ctx->fileHeader, sizeof(BMP_file_header));
    write(fd_out, &ctx->infoHeader, sizeof(BMP_info_header));
    write(fd_out, ctx->pixels, ctx->rowSize * ctx->height);

    close(fd_out);
    free(ctx->pixels);

    turn = 3;
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int main()
{
#if 0
    BMPCONTEXT *ctx = (BMPCONTEXT *)malloc(sizeof(BMPCONTEXT));
    /* TODO: 缺少非空检查 */
    memset(ctx, 0, sizeof(BMPCONTEXT));

    pthread_t input;
    pthread_t process;
    pthread_t output;
#else
    pthread_t input   = 0;
    pthread_t process = 0;
    pthread_t output  = 0;
    BMPCONTEXT *ctx = NULL;

    ctx = (BMPCONTEXT *)malloc(sizeof(BMPCONTEXT));
    if (ctx == NULL) {
        log_error("malloc %d failed\n", sizeof(BMPCONTEXT));
        return -1;
    }
    memset(ctx, 0, sizeof(BMPCONTEXT));
#endif

    pthread_create(&input, NULL, input_bmp, ctx);
    pthread_create(&process, NULL, process_bmp, ctx);
    pthread_create(&output, NULL, output_bmp, ctx);

    pthread_join(input, NULL);
    pthread_join(process, NULL);
    pthread_join(output, NULL);

    free(ctx);
    return 0;

}

