#pragma once

#ifndef BHDLL_d
#define BHDLL_d

//宏定义导出
#ifdef BHDLL_EXPORTS//如果没有定义DLLH 就定义 DLLH __declspec(dllexport)
#define BHDLL __declspec(dllexport)//导出
#else
#define BHDLL __declspec(dllimport)//导入
#endif // DLLH__//如果没有定义DLLH 就定义 DLLH 

//编写代码区域

//导出函数

BHDLL int add(int a, int b);
BHDLL int sub(int a, int b);

//导出类

class BHDLL dllH
{
public:
	int mul(int a, int b);
	int div(int a, int b);
};

//以C语言方式导出函数：
extern "C"
{
	BHDLL int Cadd(int a, int b);
	BHDLL int Csub(int a, int b);
}



#endif