#include <cstdlib>
#include <iostream>
#include <dlfcn.h>

int main(){
    int num_of_lib; /*чекаем правильность введненый библиотеки*/
    std::cout << "Введите номер библиотеки: ";
    std::cin >> num_of_lib;
    if(num_of_lib != 1 && num_of_lib != 2){
        std::cout << "ошибочная библиотека\n";
        exit(1);
    }

    --num_of_lib; /*уменьшаем число нашей либы*/
    int command;
    const char* libs[] = {"d1.so", "d2.so"}; /*выбираем нужную либу*/
    void* library_handle;
    library_handle = dlopen (libs[num_of_lib], RTLD_LAZY); 
    /*флаг - разрешение символов будет производиться только при первом обращении к соответствующей функции в библиотеке
    Загружает динамическую библиотеку с указанным именем и возвращает указатель на обработчик библиотеки*/
    if(!library_handle){ /*проверим была ли загружена ф-ия с помощью dlopen()*/
        std::cout << "Ошибка при вызове функции dlopen\n";
        exit(1);
    }

    float (*Square)(float A, float B); /*указатель на функцию*/
    char* (*translation)(long x);

    Square = (float(*)(float, float))dlsym(library_handle, "Square"); /*передаем функцию из либы с помощью dlsym() - 
                                                                    получить адрес функции в динамической библиотеке по её имени*/
    translation = (char*(*)(long x))dlsym(library_handle, "translation");

    std::cout << "Введите команду 0, 1 или 2\n"; 
    while(std::cin >> command) {
        switch (command) { /*проверяем что значени корректное*/
            default:
                std::cout << "Неверная команда\n";
                break;
            case 0: /*закрываем текущую либу,переменная num_of_lib обновляется с помощью операции остатка от деления, чтобы 
            переключаться между двумя библиотеками по кругу. Далее вызывается dlopen с новой библиотекой.
            функции Square и translation обновляются, чтобы они указывали на функции из новой библиотеки.*/
                dlclose(library_handle);
                num_of_lib = (num_of_lib + 1) % 2;
                library_handle = dlopen(libs[num_of_lib], RTLD_LAZY);
                if(!library_handle){
                    std::cout << "Ошибка при вызове функции dlopen\n";
                    exit(1);
                }
                Square = (float(*)(float, float))dlsym(library_handle, "Square");
                translation = (char*(*)(long x))dlsym(library_handle, "translation");
                std::cout << "Смена функций библиотеки\n";
                break;
            case 1: /*тут делаем квадрат*/
                std::cout << "Введите длину A и B: ";
                float A, B;
                std::cin >> A >> B;
                std::cout << "Квадрат - " << Square(A, B) << std::endl;
                break;
            case 2: /*тут делаем перевод в другую сс, в зависимости от выбранной либы*/
                long x;
                std::cout << "Введите десятичное число: ";
                std::cin >> x;
                char* memory = translation(x);
                if(num_of_lib + 1 == 1) {
                    std::cout << "Двоичное число - " << memory << std::endl;
                }
                else if (num_of_lib + 1 == 2){
                    std::cout << "Троичное число - " << memory << std::endl;
                }
                free(memory);
                break;
        }
    }
    dlclose(library_handle);
}