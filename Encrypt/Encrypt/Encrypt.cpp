#include "pch.h"
#include "Encrypt.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

class FileEncryptor {
private:
    static std::string getCurrentTime() {
        std::time_t now = std::time(0);
        std::tm tm;

        // 使用安全的 localtime_s 替代不安全的 localtime
        if (localtime_s(&tm, &now) != 0) {
            // 如果获取时间失败，返回默认值
            return "01010000";
        }

        std::stringstream ss;
        ss << std::setfill('0')
            << std::setw(2) << tm.tm_mday
            << std::setw(2) << tm.tm_hour
            << std::setw(2) << tm.tm_min
            << std::setw(2) << tm.tm_sec;
        return ss.str();
    }

    static std::vector<BYTE> addPKCS7Padding(const std::vector<BYTE>& data, size_t blockSize) {
        if (data.empty()) return data;

        size_t padLen = blockSize - (data.size() % blockSize);
        std::vector<BYTE> paddedData = data;
        for (size_t i = 0; i < padLen; ++i) {
            paddedData.push_back(static_cast<BYTE>(padLen));
        }
        return paddedData;
    }

    static bool encrypt3DESWithCryptoAPI(const std::vector<BYTE>& plainData,
        const std::string& key, const std::string& iv,
        std::vector<BYTE>& encryptedData) {
        HCRYPTPROV hProv = NULL;
        HCRYPTKEY hKey = NULL;
        HCRYPTHASH hHash = NULL;
        bool success = false;

        do {
            if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                break;
            }

            if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
                break;
            }

            if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(key.c_str()), static_cast<DWORD>(key.length()), 0)) {
                break;
            }

            if (!CryptDeriveKey(hProv, CALG_3DES, hHash, 0, &hKey)) {
                break;
            }

            if (!CryptSetKeyParam(hKey, KP_IV, reinterpret_cast<const BYTE*>(iv.c_str()), 0)) {
                break;
            }

            DWORD mode = CRYPT_MODE_CBC;
            if (!CryptSetKeyParam(hKey, KP_MODE, reinterpret_cast<const BYTE*>(&mode), 0)) {
                break;
            }

            // 添加PKCS7填充
            std::vector<BYTE> paddedData = addPKCS7Padding(plainData, 8);

            DWORD dataLen = static_cast<DWORD>(paddedData.size());
            encryptedData = paddedData;

            if (!CryptEncrypt(hKey, 0, TRUE, 0, encryptedData.data(), &dataLen, static_cast<DWORD>(encryptedData.size()))) {
                break;
            }

            encryptedData.resize(dataLen);
            success = true;

        } while (false);

        // 清理资源
        if (hKey) CryptDestroyKey(hKey);
        if (hHash) CryptDestroyHash(hHash);
        if (hProv) CryptReleaseContext(hProv, 0);

        return success;
    }

    static bool encryptAESWithCryptoAPI(const std::vector<BYTE>& plainData,
        const std::string& key, const std::string& iv,
        std::vector<BYTE>& encryptedData) {
        HCRYPTPROV hProv = NULL;
        HCRYPTKEY hKey = NULL;
        HCRYPTHASH hHash = NULL;
        bool success = false;

        do {
            if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
                if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                    break;
                }
            }

            if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
                break;
            }

            if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(key.c_str()), static_cast<DWORD>(key.length()), 0)) {
                break;
            }

            if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) {
                break;
            }

            if (!CryptSetKeyParam(hKey, KP_IV, reinterpret_cast<const BYTE*>(iv.c_str()), 0)) {
                break;
            }

            DWORD mode = CRYPT_MODE_CBC;
            if (!CryptSetKeyParam(hKey, KP_MODE, reinterpret_cast<const BYTE*>(&mode), 0)) {
                break;
            }

            // 添加PKCS7填充
            std::vector<BYTE> paddedData = addPKCS7Padding(plainData, 16);

            DWORD dataLen = static_cast<DWORD>(paddedData.size());
            encryptedData = paddedData;

            if (!CryptEncrypt(hKey, 0, TRUE, 0, encryptedData.data(), &dataLen, static_cast<DWORD>(encryptedData.size()))) {
                break;
            }

            encryptedData.resize(dataLen);
            success = true;

        } while (false);

        // 清理资源
        if (hKey) CryptDestroyKey(hKey);
        if (hHash) CryptDestroyHash(hHash);
        if (hProv) CryptReleaseContext(hProv, 0);

        return success;
    }

public:
    static bool encrypt3DES(const std::string& filePath, const std::string& fileName, const std::string& key) {
        try {
            // 验证密钥长度
            if (key.length() < 16) {
                return false;
            }

            // 生成IV（使用当前时间）
            std::string iv = getCurrentTime();
            if (iv.length() < 8) {
                iv.append(8 - iv.length(), '0');
            }

            // 读取原始文件
            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) {
                return false;
            }

            std::vector<BYTE> plainData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();

            if (plainData.empty()) {
                return false;
            }

            // 加密数据
            std::vector<BYTE> encryptedData;
            if (!encrypt3DESWithCryptoAPI(plainData, key, iv, encryptedData)) {
                return false;
            }

            // 生成输出文件名（格式：3DES_key_filename）
            std::string outputFileName = "3DES_" + key + "_" + fileName;
            std::string outputPath = filePath + outputFileName;

            // 写入加密文件
            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) {
                return false;
            }

            outFile.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
            outFile.close();

            return true;
        }
        catch (...) {
            return false;
        }
    }

    static bool encryptAES(const std::string& filePath, const std::string& fileName, const std::string& key) {
        try {
            // 验证密钥长度
            if (key.length() < 16) {
                return false;
            }

            // 生成IV（密钥中间8位 + 当前时间）
            std::string ctime = getCurrentTime();
            std::string iv;
            if (key.length() >= 12) {
                iv = key.substr(4, 8) + ctime;
            }
            else {
                iv = key + std::string(12 - key.length(), '0') + ctime;
            }

            if (iv.length() < 16) {
                iv.append(16 - iv.length(), '0');
            }

            // 读取原始文件
            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) {
                return false;
            }

            std::vector<BYTE> plainData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();

            if (plainData.empty()) {
                return false;
            }

            // 加密数据
            std::vector<BYTE> encryptedData;
            if (!encryptAESWithCryptoAPI(plainData, key, iv, encryptedData)) {
                return false;
            }

            // 生成输出文件名（格式：AES_key_filename）
            std::string outputFileName = "AES_" + key + "_" + fileName;
            std::string outputPath = filePath + outputFileName;

            // 写入加密文件
            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) {
                return false;
            }

            outFile.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
            outFile.close();

            return true;
        }
        catch (...) {
            return false;
        }
    }
};

// 导出的DLL函数
ENCRYPT_API bool encrypt3DES(const char* filePath, const char* fileName, const char* key) {
    if (!filePath || !fileName || !key) {
        return false;
    }
    return FileEncryptor::encrypt3DES(filePath, fileName, key);
}

ENCRYPT_API bool encryptAES(const char* filePath, const char* fileName, const char* key) {
    if (!filePath || !fileName || !key) {
        return false;
    }
    return FileEncryptor::encryptAES(filePath, fileName, key);
}

ENCRYPT_API const char* getVersion() {
    return "FileEncryptor DLL v1.0";
}

ENCRYPT_API bool initialize() {
    // 可以在这里进行初始化操作
    return true;
}

ENCRYPT_API void cleanup() {
    // 可以在这里进行清理操作
}