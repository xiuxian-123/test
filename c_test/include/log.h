#ifndef LOG_H
#define LOG_H

#define log(fmt, ...) do {\
    fprintf(stderr, "%d|%s(): ", __LINE__, __func__);\
    fprintf(stderr, fmt, ##__VA_ARGS__);\
} while (0)
#define log_error(fmt, ...) log("Error: " fmt "\n", ##__VA_ARGS__)
#define log_warn(fmt, ...)  log("Warn: "  fmt "\n", ##__VA_ARGS__)
#define log_info(fmt, ...)  log("Info: "  fmt "\n", ##__VA_ARGS__)
 


#endif


