#include <iostream>

extern "C" float Square(float A, float B);
extern "C" char* translation(long x);

int main(){
    int command;
    while((std::cout << "Введите команду: ") && (std::cin >> command)){
        if(command == 1){
            std::cout << "Введите длину A и B: ";
            float A, B;
            std::cin >> A >> B;
            std::cout << "Квадрат -  " << Square(A, B) << std::endl;
        }
        else if(command == 2){
            long x;
            std::cout << "Введите десятичное число: ";
            std::cin >> x;
            char* memory = translation(x); /*преобразование числа в бинарное\троичное и суем в переменную*/
            std::cout << "Двоичное число - " << memory << std::endl;
            free(memory);
        }
        else
            std::cout << "Команды могут быть 1 и 2 ";
    }
}
