#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define log(fmt, ...) do { \
    fprintf(stderr, "%d|%s(): ", __LINE__, __func__); \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
} while (0)

#define log_error(fmt, ...) log("Error: " fmt "\n", ##__VA_ARGS__)
#define log_warn(fmt, ...)  log("Warn: "  fmt "\n", ##__VA_ARGS__)
#define log_info(fmt, ...)  log("Info: "  fmt "\n", ##__VA_ARGS__)

typedef float (*operator_func)(float num1, float num2);

struct calculator {
    operator_func add;
    operator_func subtract;
    operator_func multiply;
    operator_func dicide;
};

float add(float num1, float num2)
{
    float result = num1 + num2;
    log_info("sum result: %.2f", result);

    return result;
}

float subtract(float num1, float num2)
{
    float result = num1 - num2;
    log_info("diff result : %.2f",result);

    return result;
}

float multiply(float num1, float num2) 
{
    float result = num1 * num2;
    log_info("prod result : %.2f", result);

    return result;
}

float dicide(float num1, float num2)
{
    if (num2 == 0) {
        log_error("invalid param");
        return -1.0f;
    }

    float result = num1 / num2;
    log_info("quot result : %.2f", result);

    return result;
}

float Result(float num1, operator_func cb, float num2) 
{
    log_info("Computting result ...");

    if (cb != NULL) {
        return cb(num1, num2);
    } else {
        /* fprintf(stderr, "Computted failed\n"); */
        log_error("Computted failed");
        return 0.0;
    }

    return 0.0f;
}


static int calculator_init(struct calculator **handle)
{
    struct calculator *calculator = NULL;

    calculator = (struct calculator *)malloc(sizeof(struct calculator));
    if (calculator == NULL) {
        log_error("malloc %zu failed\n", sizeof(struct calculator));
        return -1;
    }
    memset(calculator, 0, sizeof(struct calculator));

    calculator->add = add;
    calculator->subtract = subtract;
    calculator->multiply = multiply;
    calculator->dicide = dicide;
    
    *handle = calculator;
    return 0;
}

static int calculator_deinit(struct calculator **handle)
{
    if (handle != NULL && *handle != NULL) {
        free(*handle);
        *handle = NULL;
        return 0;
    }

    return -1;
}

int main()
{
    char buffer[256];

    char a = 0;
    float x = 0.0;
    float y = 0.0;
    struct calculator *calculator = NULL;
    
    if (calculator_init(&calculator) != 0) {
        log_error("calculator init failed");
        calculator_deinit(&calculator);
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        if (sscanf(buffer, "%f %c %f", &x, &a, &y) == 0) {
            float result = 0.0f;
            switch (a) 
            {
            case '+':
                result = calculator->add(x, y);
                break;
            case '-':
                result = calculator->subtract(x, y);
                break;
            case '*':
                result = calculator->multiply(x, y);
                break;
            case '/':
                result = calculator->dicide(x, y);
                break;
            default:
                log_error("Error: Unknow operator");
                fflush(stdout);
                continue;
            }

            printf("%.2f\n",result);
            fflush(stdout);
        } else {
            log_error("Invalid format");
            fflush(stdout);
        }
    }
    calculator_deinit(&calculator);
    return 0;
}

