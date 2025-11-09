// 使用隐式链接
#include "pch.h"
#include "MathLibrary.h"
#pragma comment(lib, "MathLibrary.lib")

int main() {
    int result = Add(5, 3);
    Calculator calc;
    int diff = calc.Subtract(10, 4);
    return 0;
}