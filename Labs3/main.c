#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>


void print_usage(char* cmd) { /* ф-ия для вывода потоков */
    printf("Использование: %s [-threads num]\n", cmd);
}

bool read_matrix(float* matrix, size_t rows, size_t cols) { /* считываем матрицу, пробегаясь по строчкам  и столбикам. Если все ок
                                                            то тру, если нет, то вызов perror для уведомления об ошибке */
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            if (scanf("%f", &matrix[i * cols + j]) != 1) {
                perror("Ошибка во время чтении матрицы");
                return false;
            }
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

void copy_matrix(float* from, float* to, size_t rows, size_t cols) { /* выводим нашу матрицу */
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            to[i * cols + j] = from[i * cols + j];
        }
    }
}

typedef struct { /* необходимые параметры для выполнения задачи в многопоточной среде */
    int thread_num; /* номер текущего потока выполнения */
    int th_count; /* общее количество потоков выполнения */
    int rows; /* количество строк в матрицах */
    int cols; /* количество столбцов в матрицах */
    int w_dim; /* размерность */
    float** matrix1; /* указатель на первую матрицу */
    float** result1; /* указатель на матрицу-результат для первой операции */
    float** matrix2; /* указатель на вторую матрицу */
    float** result2; /* указатель на матрицу-результат для второй операции */
} thread_arg;

void* edit_line(void* argument) {
    thread_arg* args = (thread_arg*)argument; /*извлекаем значение треда_арг, номер потока и кол-во потоков*/
    const int thread_num = args->thread_num;
    const int th_count = args->th_count;

    const int rows = args->rows; /*кол-во столбцов, строк и смещение*/
    const int cols = args->cols;
    int offset = args->w_dim / 2;

    float** matrix1_ptr = args->matrix1; /*извлекаем указ-ли на матрицы и на рез-тат*/
    float** matrix2_ptr = args->matrix2;
    float** result1_ptr = args->result1;
    float** result2_ptr = args->result2;

    const float* matrix1 = *matrix1_ptr; /*ук-ли на матрицы получаются из ук-телей на другие матрицы и т.д.*/
    const float* matrix2 = *matrix2_ptr; 
    float* result1 = *result1_ptr;
    float* result2 = *result2_ptr;
    /*короче редачим эле-ты матрицы*/

    for (int th_row = thread_num; th_row < rows; th_row += th_count) { /*обрабатываем мат-цу*/
        for (int th_col = 0; th_col < cols; th_col++) { /*перебираем строки матрицы с ув-ием счетчика*/
            float max = matrix1[th_row * cols + th_col];
            float min = matrix2[th_row * cols + th_col];
            for (int i = th_row - offset; i < th_row + offset + 1; i++) { /*перебираем столбцы. */
                for (int j = th_col - offset; j < th_col + offset + 1; j++) { /*ищем макс значение из мат 1 и мин из мат 2, путем перебора*/
                    float curr1, curr2;
                    if ((i < 0) || (i >= rows) || (j < 0) || (j >= cols)) {
                        curr1 = 0;
                        curr2 = 0;
                    } else {
                        curr1 = matrix1[i * cols + j];
                        curr2 = matrix2[i * cols + j];
                    }
                    if (curr1 > max) {
                        max = curr1;
                    }
                    if (curr2 < min) {
                        min = curr2;
                    }
                }
            }

            result1[th_row * cols + th_col] = max; /*сюда макс значение*/
            result2[th_row * cols + th_col] = min; /*сюда мин значение*/
        }
    }
    pthread_exit(NULL); // Заканчиваем поток
}

/*фильтруем матрицу*/
void put_filters(float** matrix_ptr, size_t rows, size_t cols, size_t w_dim, float** res1_ptr, float** res2_ptr, int filter_cnt, int th_count) {
    float* tmp1 = (float*)malloc(rows * cols * sizeof(float)); /*создаем массив пам-ти для первой исход матрицы*/
    if (!tmp1) {
        perror("Ошибка во время распределения матрицы\n");
        exit(1);
    }
    float** matrix1_ptr = &tmp1;

    float* tmp2 = (float*)malloc(rows * cols * sizeof(float));
    if (!tmp2) {
        perror("Ошибка во время распределения матрицы\n");
        exit(1);
    }
    float** matrix2_ptr = &tmp2;
    copy_matrix(*matrix_ptr, tmp1, rows, cols);
    copy_matrix(*matrix_ptr, tmp2, rows, cols);

    pthread_t ids[th_count]; /*сюда пар-ты для обработки мат-цы*/
    thread_arg args[th_count]; /*сюда укат-ли на матр, пар-ты обраб-ки, номер потока*/

    for (int k = 0; k < filter_cnt; k++) { /*прим-ем фильтры на матрицу. Для каждого фил-ра создаем набор пот-ов == каунтеру*/
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

            if (pthread_create(&ids[i], NULL, edit_line, &args[i]) != 0) { /*не смогли соз-ть поток*/
                perror("Не могу создать поток.\n");
            }
        }

        for(int i = 0; i < th_count; i++) { /*ждем завершения работы всех потоков*/
            if (pthread_join(ids[i], NULL) != 0) {
                perror("Не могу дождаться завершения потока\n");
            }
        }

        /*переставляем указ-ли на массивы рез-тата и исход матрицы, если в ф-ию передано больше 1-го фильтра*/
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
        printf("Введите K = "); /*вводим значение*/
        int threads;
        scanf("%d", &threads);

    if (argc == 3) { /*если 3 аргум, то значение 3го аргум испол как нов знач перемен threads*/
        threads = atoi(argv[2]);
    } else if (argc != 1) { /*если др-ое знач, выз-ам ф-ию принта и оффаем раб-ту*/
        print_usage(argv[0]);
        return 0;
    }

    int rows;
    int cols;
    printf("Введите размерность матрицы:\n"); /*получаем размер матрицы*/
    scanf("%d", &cols);
    scanf("%d", &rows);
    float* matrix = (float*)malloc(rows * cols * sizeof(float));
    float* res1 = (float*)malloc(rows * cols * sizeof(float));
    float* res2 = (float*)malloc(rows * cols * sizeof(float));
    if (!matrix || !res1 || !res2) {
        perror("Ошибка во время распределения матрицы\n");
        return 1;
    }
    read_matrix(matrix, rows, cols); /*заполн матрицу нашими значениями*/

    /*получаем раз-ть окна, кол-во примения эрозии+наращивания к матр.*/
    int w_dim;
    printf("Введите размерность окна:\n");
    scanf("%d", &w_dim);
    if (w_dim % 2 == 0) {
        perror("Размер окна должен быть нечетным числом\n"); /*почему нечет? Чтобы было симметрич относит центра*/
        return 1;
    }

    printf("Результат \n");
    int k;
    scanf("%d", &k);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    put_filters(&matrix, rows, cols, w_dim, &res1, &res2, k, threads);

    gettimeofday(&end, NULL);

    long sec = end.tv_sec - start.tv_sec; /*вычисление времени размеры команды*/
    long microsec = end.tv_usec - start.tv_usec;
    if (microsec < 0) {
        --sec;
        microsec += 1000000;
    }
    long elapsed = sec*1000000 + microsec;


    printf("Наращивание:\n");
    print_matrix(res1, rows, cols);
    printf("Эрозия:\n");
    print_matrix(res2, rows, cols);
    printf("Общее время: %ld ms\n", elapsed);

    free(res1);
    free(res2);
    free(matrix);
    return 0;
}
