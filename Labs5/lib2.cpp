#include <iostream>
#include <algorithm>
#include <cstring>

extern "C" float Square(float A, float B);
extern "C" char* translation(long x);

float Square(float A, float B){
    return 0.5 * (A * B);
}

char* translation(long x) {
    std::string bin;
    if(x == 0) bin += "0";
    while (x > 0) {
        bin += std::to_string(x % 3);
        x /= 3;
    }
    std::string number = bin;
    std::reverse(number.begin(), number.end());
    char *answer = (char *) malloc((number.size() + 1) * sizeof(char));
    strcpy(answer, number.c_str());
    return answer;
}