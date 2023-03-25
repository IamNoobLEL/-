#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


int main(int argc, char *argv[]) {
    
    int pac = atoi(argv[1]); 
    int pca = atoi(argv[2]); 
    int pcb = atoi(argv[3]);
    
    size_t size;

    while (read(pac, &size, sizeof(size_t)) > 0) { /*читаем размер строки из пак и выделяем память*/
        char *str = (char*) malloc(size);  
        if (str == NULL) {
            printf("Выделение памяти из С\n"); 
            exit(-1);
        }
        read(pac, str, size); /*читаем*/
        printf("C - из a: %s\n", str); /*выводим строку*/
        write(pcb, &size, sizeof(size_t)); /*записываем размер в канал связанный с B*/
        int ok = 1; /*говорим процессу А, что он может читать след строку*/
        write(pca, &ok, sizeof(int));
        free(str);
    }

    close(pac);
    close(pca);
    close(pcb);


    return 0;
}
