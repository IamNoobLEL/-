#include <iostream>
#include <algorithm>
#include <cstring>

extern "C" float Square(float A, float B);
extern "C" char* translation(long x);

float Square(float A, float B){
    return A * B;
}

char* translation(long x){
    std::string bin;
    if(x == 0) bin += "0";
    while(x != 0){
        if(x % 2 ==0)
        {
            x/=2;
            bin += "0";
        }
        else
        if(x % 2 != 0)
        {
            x /= 2;
            bin += "1";
        }
    }
    std::string number = bin;
    std::reverse(number.begin(), number.end());
    char* answer = (char*) malloc((number.size() + 1) * sizeof(char));
    strcpy(answer, number.c_str());
    return answer;
}