#include "BH_dll.h"
#include <iostream>

using namespace std;

int add(int a, int b)
{
	return a + b;
}
int sub(int a, int b)
{
	return a - b;
}

int dllH::mul(int a, int b)
{
	return a * b;
}
int dllH::div(int a, int b)
{
	return a / b;
}

int Cadd(int a, int b)
{
	return a + b;
}
int Csub(int a, int b)
{
	return a - b;
}