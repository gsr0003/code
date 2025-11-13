#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>

typedef bool(*Encrypt3DESFunc)(const char*, const char*, const char*);
typedef bool(*EncryptAESFunc)(const char*, const char*, const char*);
typedef const char* (*GetVersionFunc)();
typedef bool(*InitializeFunc)();
typedef void(*CleanupFunc)();
int main() {
    // 加载DLL
    HMODULE hDll = LoadLibraryA("../x64/Debug/Encrypt.dll");
    if (!hDll) {
        std::cout << "无法加载DLL!" << std::endl;
        return 1;
    }
    /*
    // 获取函数地址
    GetVersionFunc getVersion = (GetVersionFunc)GetProcAddress(hDll, "getVersion");
    InitializeFunc initialize = (InitializeFunc)GetProcAddress(hDll, "initialize");
    CleanupFunc cleanup = (CleanupFunc)GetProcAddress(hDll, "cleanup");
    Decrypt3DESFunc decrypt3DES = (Decrypt3DESFunc)GetProcAddress(hDll, "decrypt3DES");
    DecryptAESFunc decryptAES = (DecryptAESFunc)GetProcAddress(hDll, "decryptAES");

    if (!getVersion || !initialize || !cleanup || !decrypt3DES || !decryptAES) {
        std::cout << "无法获取函数地址!" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    std::cout << "DLL版本: " << getVersion() << std::endl;

    // 初始化
    if (!initialize()) {
        std::cout << "初始化失败!" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    // 创建测试目录和文件
    CreateDirectoryA("./encrypted", NULL);
    CreateDirectoryA("./decrypted", NULL);

    std::ofstream testFile1("./encrypted/prefix_16bytekey123456_suffix.dat", std::ios::binary);
    std::ofstream testFile2("./encrypted/pre_16bytekey1234567_suffix.dat", std::ios::binary);

    if (testFile1) {
        testFile1 << "Test 3DES file content";
        testFile1.close();
    }
    if (testFile2) {
        testFile2 << "Test AES file content";
        testFile2.close();
    }

    // 测试解密功能
    bool result1 = decrypt3DES("./encrypted/", "3DES_XyH5xUsHlaJckrCW_huffman_1.txt.zip", "./decrypted/");
    bool result2 = decryptAES("./encrypted/", "AES_oWuqDCuKuZoCZxGJ_LZMA_1.txt.tgz", "./decrypted/");

    std::cout << "3DES解密: " << (result1 ? "成功" : "失败") << std::endl;
    std::cout << "AES解密: " << (result2 ? "成功" : "失败") << std::endl;

    // 清理
    cleanup();
    FreeLibrary(hDll);

    std::cout << "按任意键退出..." << std::endl;
    std::cin.get();

    return 0;
    */
}