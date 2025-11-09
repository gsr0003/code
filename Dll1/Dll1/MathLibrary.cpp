// MathLibrary.cpp
#include "pch.h" // 预编译头文件
#include "MathLibrary.h"

// 简单函数实现
MATHLIBRARY_API int Add(int a, int b) {
    return a + b;
}

MATHLIBRARY_API int Multiply(int a, int b) {
    return a * b;
}

// 类方法实现
int Calculator::Subtract(int a, int b) {
    return a - b;
}

double Calculator::Divide(double a, double b) {
    if (b == 0) return 0;
    return a / b;
}