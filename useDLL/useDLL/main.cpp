#include <iostream>

using namespace std;
#include <BH_dll.h>

int main()
{
	cout << add(1, 0) << endl;
	cout << sub(1, 0) << endl;

	dllH DH;
	cout << DH.div(4, 2) << endl;
	cout << DH.mul(4, 2) << endl;

	cout << Cadd(1, 0) << endl;
	cout << Csub(1, 0) << endl;
	getchar();
	return 0;
}