#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    int pcb = atoi(argv[1]);
    int pab = atoi(argv[2]);  
    
    size_t size;

    while (read(pab, &size, sizeof(size_t)) > 0) { // ждёт от А размер
        // как только программа А завершится, выход из while
        printf("B - From a: %zu\n", size);
        read(pcb, &size, sizeof(size_t));
        printf("B - From c: %zu\n", size);
    }

    close(pcb);
    close(pab);
    
    return 0;
}