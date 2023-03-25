#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

int main(int argc, char *argv[]) { /*принимаем аргументы командной строки. argc - это количество аргументов командной строки, 
                                    а argv[] - массив указателей на строки, представляющие собой эти аргументы.*/
    int pcb = atoi(argv[1]); /*извлекаем 2 аргумента и преобразуем их в целочисленные значения с помощью функции atoi()*/
    int pab = atoi(argv[2]);  
    
    size_t size;

    while (read(pab, &size, sizeof(size_t)) > 0) { /*ждем от А размер*/
        /*как только А завершится, выход из цикла*/
        printf("B - из a: %zu\n", size); /*выводим что получили размер из А*/
        read(pcb, &size, sizeof(size_t)); /*ждем размер от С*/
        printf("B - из c: %zu\n", size); /*выводим*/
    }

    close(pcb);
    close(pab);
    
    return 0;
}
