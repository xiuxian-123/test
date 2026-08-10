#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <log.h>

#define BM_MAGIC   (0x4D42)
#define BLOCK_SIZE (20)
#define WORKER_NUM (4)
#define MAX_TASKS  (65536)

#pragma pack(push, 1)
struct BMP_file_header {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

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

/* 一个 20x20 马赛克块的任务描述 */
typedef struct {
    int x;
    int y;
} Task;

/* 动态任务队列:所有块一次性入队,worker 抢任务并行处理 */
typedef struct {
    Task tasks[MAX_TASKS];
    int  head;
    int  tail;
    int  count;
    int  all_done;              /* 入队完毕标志:队列空且 all_done 时 worker 退出 */
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} TaskQueue;

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int rowSize;
    struct BMP_file_header fileHeader;
    struct BMP_info_header infoHeader;
    TaskQueue queue;            /* 任务队列(共享状态,由 queue.lock 保护) */
    int processed_blocks;       /* 已处理块数(统计,同一把锁保护) */
} BMPCONTEXT;

/* 阶段1:初始化 —— open/read/lseek 读入 input.bmp,校验 24 位格式,申请像素内存 */
static int init_bmp(BMPCONTEXT *ctx)
{
    const char *input_path = "input.bmp";
    int fd_in = -1;
    int total_blocks = 0;

    fd_in = open(input_path, O_RDONLY);
    if (fd_in == -1) {
        log_error("open %s failed\n", input_path);
        return -1;
    }

    if (read(fd_in, &ctx->fileHeader, sizeof(struct BMP_file_header)) != sizeof(struct BMP_file_header)) {
        log_error("read file header failed\n");
        goto fail_close;
    }
    if (read(fd_in, &ctx->infoHeader, sizeof(struct BMP_info_header)) != sizeof(struct BMP_info_header)) {
        log_error("read info header failed\n");
        goto fail_close;
    }

    if (ctx->fileHeader.bfType != BM_MAGIC || ctx->infoHeader.biBitCount != 24) {
        log_error("just 24bit BMP\n");
        goto fail_close;
    }

    ctx->width   = ctx->infoHeader.biWidth;
    ctx->height  = abs(ctx->infoHeader.biHeight);
    ctx->rowSize = ((ctx->width * 3 + 3) / 4) * 4;

    /* 校验任务总数不超出队列容量,防止数组越界 */
    total_blocks = ((ctx->height + BLOCK_SIZE - 1) / BLOCK_SIZE) * ((ctx->width + BLOCK_SIZE - 1) / BLOCK_SIZE);
    if (total_blocks > MAX_TASKS) {
        log_error("blocks %d exceed MAX_TASKS %d\n", total_blocks, MAX_TASKS);
        goto fail_close;
    }

    ctx->pixels = (uint8_t*)malloc(ctx->rowSize * ctx->height);
    if (ctx->pixels == NULL) {
        log_error("malloc %d failed\n", ctx->rowSize * ctx->height);
        goto fail_close;
    }
    memset(ctx->pixels, 0, ctx->rowSize * ctx->height);

    if (lseek(fd_in, ctx->fileHeader.bfOffBits, SEEK_SET) == -1) {
        log_error("lseek failed\n");
        goto fail_free;
    }
    if (read(fd_in, ctx->pixels, ctx->rowSize * ctx->height) != ctx->rowSize * ctx->height) {
        log_error("read pixels failed\n");
        goto fail_free;
    }

    close(fd_in);
    return 0;

fail_free:
    free(ctx->pixels);
    ctx->pixels = NULL;
fail_close:
    close(fd_in);
    return -1;
}

/* 主线程:把全部 20x20 块一次性入队(此时 worker 尚未创建,加锁保证队列操作统一规范) */
static void enqueue_all_blocks(BMPCONTEXT *ctx)
{
    int y = 0;

    for (y = 0; y < ctx->height; y += BLOCK_SIZE) {
        for (int x = 0; x < ctx->width; x += BLOCK_SIZE) {
            pthread_mutex_lock(&ctx->queue.lock);
            ctx->queue.tasks[ctx->queue.tail].x = x;
            ctx->queue.tasks[ctx->queue.tail].y = y;
            ctx->queue.tail++;
            ctx->queue.count++;
            pthread_mutex_unlock(&ctx->queue.lock);
        }
    }
}

/* 处理一个 20x20 块:求 B/G/R 各通道均值并写回,即马赛克效果 */
static void process_block(BMPCONTEXT *ctx, int x, int y)
{
    int end_y = (y + BLOCK_SIZE > ctx->height) ? ctx->height : (y + BLOCK_SIZE);
    int end_x = (x + BLOCK_SIZE > ctx->width) ? ctx->width : (x + BLOCK_SIZE);

    int sum_b = 0;
    int sum_g = 0;
    int sum_r = 0;
    int count = 0;

    for (int by = y; by < end_y; by++) {
        for (int bx = x; bx < end_x; bx++) {
            int index = by * ctx->rowSize + bx * 3;
            sum_b += ctx->pixels[index];
            sum_g += ctx->pixels[index + 1];
            sum_r += ctx->pixels[index + 2];
            count++;
        }
    }

    uint8_t avg_b = (uint8_t)(sum_b / count);
    uint8_t avg_g = (uint8_t)(sum_g / count);
    uint8_t avg_r = (uint8_t)(sum_r / count);

    for (int by = y; by < end_y; by++) {
        for (int bx = x; bx < end_x; bx++) {
            int index = by * ctx->rowSize + bx * 3;
            ctx->pixels[index]     = avg_b;
            ctx->pixels[index + 1] = avg_g;
            ctx->pixels[index + 2] = avg_r;
        }
    }
}

/* worker 线程:抢任务处理。队列空且入队完毕则退出;while+cond_wait 防虚假唤醒 */
static void *worker_bmp(void *arg)
{
    BMPCONTEXT *ctx = (BMPCONTEXT *)arg;

    while (1) {
        Task task;
        int has_task = 0;

        pthread_mutex_lock(&ctx->queue.lock);
        while (ctx->queue.count == 0 && ctx->queue.all_done == 0) {
            pthread_cond_wait(&ctx->queue.cond, &ctx->queue.lock);
        }
        if (ctx->queue.count > 0) {
            task = ctx->queue.tasks[ctx->queue.head];
            ctx->queue.head++;
            ctx->queue.count--;
            has_task = 1;
        }
        pthread_mutex_unlock(&ctx->queue.lock);

        if (has_task == 0) {
            break;
        }

        /* 数据分区:每个块只会被一个 worker 取走,写各自像素区域,无需加锁 */
        process_block(ctx, task.x, task.y);

        /* 共享统计:必须加锁 */
        pthread_mutex_lock(&ctx->queue.lock);
        ctx->processed_blocks++;
        pthread_mutex_unlock(&ctx->queue.lock);
    }

    return NULL;
}

/* 阶段3:去初始化 —— 更新头部、写出 output.bmp、释放像素内存 */
static int deinit_bmp(BMPCONTEXT *ctx)
{
    const char *output_path = "output.bmp";
    int fd_out = -1;
    int ret = 0;

    ctx->fileHeader.bfSize = sizeof(struct BMP_file_header) + sizeof(struct BMP_info_header) + (ctx->rowSize * ctx->height);
    ctx->infoHeader.biSizeImage = ctx->rowSize * ctx->height;

    fd_out = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd_out == -1) {
        log_error("open %s failed\n", output_path);
        ret = -1;
        goto out_free;
    }

    if (write(fd_out, &ctx->fileHeader, sizeof(struct BMP_file_header)) != sizeof(struct BMP_file_header)) {
        log_error("write file header failed\n");
        goto out_close;
    }
    if (write(fd_out, &ctx->infoHeader, sizeof(struct BMP_info_header)) != sizeof(struct BMP_info_header)) {
        log_error("write info header failed\n");
        goto out_close;
    }
    if (write(fd_out, ctx->pixels, ctx->rowSize * ctx->height) != ctx->rowSize * ctx->height) {
        log_error("write pixels failed\n");
        goto out_close;
    }

out_close:
    close(fd_out);
out_free:
    free(ctx->pixels);
    ctx->pixels = NULL;
    return ret;
}

int main()
{
    BMPCONTEXT *ctx = NULL;
    pthread_t workers[WORKER_NUM] = {0};
    int created = 0;
    int total_blocks = 0;

    ctx = (BMPCONTEXT *)malloc(sizeof(BMPCONTEXT));
    if (ctx == NULL) {
        log_error("malloc %zu failed\n", sizeof(BMPCONTEXT));
        return -1;
    }
    memset(ctx, 0, sizeof(BMPCONTEXT));

    /* 阶段1:初始化(资源申请) */
    if (init_bmp(ctx) != 0) {
        goto fail;
    }

    /* 阶段2:处理(真正多线程) */
    total_blocks = ((ctx->height + BLOCK_SIZE - 1) / BLOCK_SIZE) * ((ctx->width + BLOCK_SIZE - 1) / BLOCK_SIZE);
    enqueue_all_blocks(ctx);

    for (int i = 0; i < WORKER_NUM; i++) {
        if (pthread_create(&workers[i], NULL, worker_bmp, ctx) != 0) {
            log_error("pthread_create worker %d failed\n", i);
            break;
        }
        created++;
    }

    /* 全部任务已入队:置 all_done 并广播,唤醒空等中的 worker */
    pthread_mutex_lock(&ctx->queue.lock);
    ctx->queue.all_done = 1;
    pthread_cond_broadcast(&ctx->queue.cond);
    pthread_mutex_unlock(&ctx->queue.lock);

    for (int i = 0; i < created; i++) {
        pthread_join(workers[i], NULL);
    }

    log_info("processed %d/%d blocks\n", ctx->processed_blocks, total_blocks);

    /* 阶段3:去初始化(输出产物、资源释放) */
    if (deinit_bmp(ctx) != 0) {
        goto fail;
    }

    free(ctx);
    return 0;

fail:
    free(ctx);
    return -1;
}
