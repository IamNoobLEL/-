#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


int main(int argc, char *argv[]) {
    // argv[0] = ".\c";
    // argv[1] = 3
    // argv[2] = 4
    // argv[3] = 5
    
    // atoi: строку переводит в int
    int pac = atoi(argv[1]); 
    int pca = atoi(argv[2]); 
    int pcb = atoi(argv[3]);
    
    size_t size;

    while (read(pac, &size, sizeof(size_t)) > 0) { // ждёт от программы А размер
        char *str = (char*) malloc(size); // 
        if (str == NULL) {
            printf("MALLOC from C\n");
            exit(-1);
        }
        read(pac, str, size); // ждёт от программы А строку
        printf("C - From a: %s\n", str);
        write(pcb, &size, sizeof(size_t)); 
        int ok = 1;
        write(pca, &ok, sizeof(int));
        free(str);
    }

    close(pac);
    close(pca);
    close(pcb);


    return 0;
}