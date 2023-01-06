#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>


void print_usage(char* cmd) {
    printf("Usage: %s [-threads num]\n", cmd);
}

bool read_matrix(float* matrix, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            if (scanf("%f", &matrix[i * cols + j]) != 1) {
                perror("Error while reading matrix");
                return false;
            }
            //scanf("%f", &matrix[i*cols + j]);
            //printf("=== matrix[%ld] = %f\n", i*cols + j, matrix[i*cols + j]);
        }
    }
    return true;;
}

bool print_matrix(float* matrix, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            printf("%.20g ", matrix[i * cols + j]);
        }
        printf("\n");
    }
    return false;
}

void copy_matrix(float* from, float* to, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            to[i * cols + j] = from[i * cols + j];
        }
    }
}

typedef struct {
    int thread_num;
    int th_count;
    int rows;
    int cols;
    int w_dim;
    float** matrix1;
    float** result1;
    float** matrix2;
    float** result2;
} thread_arg;

void* edit_line(void* argument) {
    thread_arg* args = (thread_arg*)argument;
    const int thread_num = args->thread_num;
    const int th_count = args->th_count;

    const int rows = args->rows;
    const int cols = args->cols;
    int offset = args->w_dim / 2;

    float** matrix1_ptr = args->matrix1;
    float** matrix2_ptr = args->matrix2;
    float** result1_ptr = args->result1;
    float** result2_ptr = args->result2;

    const float* matrix1 = *matrix1_ptr;
    const float* matrix2 = *matrix2_ptr;
    float* result1 = *result1_ptr;
    float* result2 = *result2_ptr;

    //printf("\n=== IN THREAD %d ===\n", thread_num);
    // printf("offset = %d\n", offset);

    for (int th_row = thread_num; th_row < rows; th_row += th_count) {
        //printf("THREAD %d ROW  %d\n", thread_num, th_row);
        for (int th_col = 0; th_col < cols; th_col++) {
            // printf(" th_col = %d\n", th_col);
            float max = matrix1[th_row * cols + th_col];
            float min = matrix2[th_row * cols + th_col];
            for (int i = th_row - offset; i < th_row + offset + 1; i++) {
                for (int j = th_col - offset; j < th_col + offset + 1; j++) {
                    float curr1, curr2;
                    if ((i < 0) || (i >= rows) || (j < 0) || (j >= cols)) {
                        curr1 = 0;
                        curr2 = 0;
                    } else {
                        curr1 = matrix1[i * cols + j];
                        curr2 = matrix2[i * cols + j];
                    }
                    // printf("[%d][%d] ", i, j);
                    if (curr1 > max) {
                        max = curr1;
                    }
                    if (curr2 < min) {
                        min = curr2;
                    }
                }
                // printf("\n");
            }

            result1[th_row * cols + th_col] = max;
            result2[th_row * cols + th_col] = min;
        }
        //printf("\n");
    }
    pthread_exit(NULL); // Заканчиваем поток
}

void put_filters(float** matrix_ptr, size_t rows, size_t cols, size_t w_dim, float** res1_ptr, float** res2_ptr, int filter_cnt, int th_count) {
    float* tmp1 = (float*)malloc(rows * cols * sizeof(float));
    if (!tmp1) {
        perror("Error while allocating matrix\n");
        exit(1);
    }
    float** matrix1_ptr = &tmp1;
    float* tmp2 = (float*)malloc(rows * cols * sizeof(float));
    if (!tmp2) {
        perror("Error while allocating matrix\n");
        exit(1);
    }
    float** matrix2_ptr = &tmp2;
    copy_matrix(*matrix_ptr, tmp1, rows, cols);
    copy_matrix(*matrix_ptr, tmp2, rows, cols);

    pthread_t ids[th_count];
    thread_arg args[th_count];

    for (int k = 0; k < filter_cnt; k++) {
        for (int i = 0; i < th_count; i++) {
            args[i].thread_num = i;
            args[i].th_count = th_count;
            args[i].rows = rows;
            args[i].cols = cols;
            args[i].w_dim = w_dim;
            args[i].matrix1 = matrix1_ptr;
            args[i].result1 = res1_ptr;
            args[i].matrix2 = matrix2_ptr;
            args[i].result2 = res2_ptr;

            if (pthread_create(&ids[i], NULL, edit_line, &args[i]) != 0) {
                perror("Can't create a thread.\n");
            }
        }

        for(int i = 0; i < th_count; i++) {
            if (pthread_join(ids[i], NULL) != 0) {
                perror("Can't wait for thread\n");
            }
        }

        if (filter_cnt > 1) {
            float** swap = res1_ptr;
            res1_ptr = matrix1_ptr;
            matrix1_ptr = swap;

            swap = res2_ptr;
            res2_ptr = matrix2_ptr;
            matrix2_ptr = swap;
        }
    }

    free(tmp1);
    free(tmp2);
}

int main(int argc, char* argv[]) {
        printf("Enter K = ");
        int threads;
        scanf("%d", &threads);

    if (argc == 3) {
        threads = atoi(argv[2]);
    } else if (argc != 1) {
        print_usage(argv[0]);
        return 0;
    }

    int rows;
    int cols;
    printf("Enter matrix dimensions:\n");
    scanf("%d", &cols);
    scanf("%d", &rows);
    float* matrix = (float*)malloc(rows * cols * sizeof(float));
    float* res1 = (float*)malloc(rows * cols * sizeof(float));
    float* res2 = (float*)malloc(rows * cols * sizeof(float));
    if (!matrix || !res1 || !res2) {
        perror("Error while allocating matrix\n");
        return 1;
    }
    read_matrix(matrix, rows, cols);

    int w_dim;
    printf("Enter window dimension:\n");
    scanf("%d", &w_dim);
    if (w_dim % 2 == 0) {
        perror("Window dimension must be an odd number\n");
        return 1;
    }

    printf("Result \n");
    int k;
    scanf("%d", &k);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    put_filters(&matrix, rows, cols, w_dim, &res1, &res2, k, threads);

    gettimeofday(&end, NULL);

    long sec = end.tv_sec - start.tv_sec;
    long microsec = end.tv_usec - start.tv_usec;
    if (microsec < 0) {
        --sec;
        microsec += 1000000;
    }
    long elapsed = sec*1000000 + microsec;


    printf("Dilation:\n");
    print_matrix(res1, rows, cols);
    printf("Erosion:\n");
    print_matrix(res2, rows, cols);
    printf("Total time: %ld ms\n", elapsed);

    free(res1);
    free(res2);
    free(matrix);
    return 0;
}