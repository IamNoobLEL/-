#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

#define MIN_CAP 4
#define STDIN 0

size_t read_string(char **str_, int fd) { /*читаем строку из файл. дескрип*/
    free(*str_);
    size_t str_size = 0;
    size_t cap = MIN_CAP; /*емкость буфера для чтения строки*/
    char *str = (char*) malloc(sizeof(char) * cap); /*буфер для чтения памяти маллок возвращет указатель на первый байт памяти*/
    if (str == NULL) { /*проверка что маллок успешно все сделал*/
        perror("Ошибка выделения памяти");
        exit(-1);
    }
    char c;
    while (read(fd, &c, sizeof(char)) == 1) {
        if (c == '\n') { /*читаем строку из ФД*/
            break;
        }
        str[(str_size)++] = c; /*читаем символ из ФД и добавляем в строку*/
        if (str_size == cap) {
            str = (char*) realloc(str, sizeof(char) * cap * 3 / 2); /*если размер == вместимость, то * 1,5 раз*/
            cap = cap * 3 / 2;
            if (str == NULL) { /*если косяк в выделении памяти*/
                perror("Ошибка перераспределения памяти");
                exit(-2);
            }
        }
    }
    str[str_size] = '\0';

    *str_ = str; 
    return str_size; /*возврат размер считанной строки*/
}

size_t str_length(char *str) { /*считываем длину строки до \0*/
    size_t length = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        ++length;
    }
    return length;
}

int main() { /*создаем 4 канала*/
    int ab[2];
    int ac[2];
    int ca[2];
    int cb[2];
    
    
    pipe(ab);
    pipe(ac);
    pipe(ca);
    pipe(cb);

    int id1 = fork();

    if (id1 < 0) {
        perror("Ошибка создания процесса");
        exit(1);
    }
    else if (id1 == 0) { /*создает дочерний процесс, внутри которого будет запущена 
                            программа c.c с аргументами ac[0], ca[1], и cb[1].*/
        close(ac[1]); /*закрываем ненужные дескрипторы*/
        close(ca[0]);
        close(cb[0]);
        close(ab[0]);
        close(ab[1]);

        char pac[3];
        sprintf(pac, "%d", ac[0]); /*преобразование целочисленных значений 
                                        файловых дескрипторов в строки с помощью функции sprintf()*/

        char pca[3];
        sprintf(pca, "%d", ca[1]); 

        char pcb[3];
        sprintf(pcb, "%d", cb[1]); 
        
        execl("./c", "./c", pac, pca, pcb, NULL); /*системный вызова execl(), где первый аргумент - это имя программы, 
        которую необходимо выполнить, второй аргумент - это имя программы, которое будет использоваться 
        в качестве argv[0] в дочернем процессе, остальные аргументы - это аргументы, передаваемые в новую программу.*/
    }
    else { 
        int id2 = fork(); /*второй процесс для B*/
        if (id2 < 0) {
            perror("Ошибка создания процесса");
            exit(1);
        }
        else if (id2 == 0) {
            close(ac[0]);
            close(ac[1]);
            close(ca[0]);
            close(ca[1]);
            close(cb[1]);
            close(ab[1]);

            char pcb[2];
            sprintf(pcb, "%d", ca[0]); /*аналогично*/

            char pab[2];
            sprintf(pab, "%d", cb[0]);

            execl("./b", "./b",  pcb, pab, NULL); 
        }
        else { 
            close(ac[0]);
            close(ca[1]);
            close(ab[0]);
            close(cb[1]);
            close(cb[0]);
         
            char *str = NULL; 
            while ((read_string(&str, STDIN)) > 0) { /*читаем строки через read и передаем их через каналы взаимодействия
                                                        между процессами с помощью write и read*/
                size_t size = str_length(str); 
                write(ac[1], &size, sizeof(size_t)); 
                write(ac[1], str, size); 
                write(ab[1], &size, sizeof(size_t)); 

                int ok; 
                read(ca[0], &ok, sizeof(ok)); 
            }

            close(ca[0]);
            close(ac[1]);
            close(ab[1]);
         }
    }

    return 0;
}
