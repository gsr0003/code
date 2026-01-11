#include "pch.h"
#include "Decrypt.h"
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

class FileDecryptor {
private:
    static bool getFileCreationTime(const std::string& filepath, std::tm& creationTime) {
        HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        FILETIME ftCreate, ftAccess, ftWrite;
        if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
            CloseHandle(hFile);
            return false;
        }

        SYSTEMTIME st;
        FileTimeToSystemTime(&ftCreate, &st);

        creationTime.tm_year = st.wYear - 1900;
        creationTime.tm_mon = st.wMonth - 1;
        creationTime.tm_mday = st.wDay;
        creationTime.tm_hour = st.wHour;
        creationTime.tm_min = st.wMinute;
        creationTime.tm_sec = st.wSecond;

        CloseHandle(hFile);
        return true;
    }

    static std::string generateTimeString(const std::tm& timeinfo) {
        std::stringstream ss;
        ss << std::setfill('0')
            << std::setw(2) << timeinfo.tm_mday
            << std::setw(2) << timeinfo.tm_hour
            << std::setw(2) << timeinfo.tm_min
            << std::setw(2) << timeinfo.tm_sec;
        return ss.str();
    }

    static std::vector<BYTE> removePKCS7Padding(const std::vector<BYTE>& data) {
        if (data.empty()) return data;

        BYTE padValue = data[data.size() - 1];
        if (padValue > data.size()) return data;

        for (size_t i = data.size() - padValue; i < data.size(); ++i) {
            if (data[i] != padValue) return data;
        }

        return std::vector<BYTE>(data.begin(), data.end() - padValue);
    }

    static bool decrypt3DESWithCryptoAPI(const std::vector<BYTE>& encryptedData,
        const std::string& key, const std::string& iv,
        std::vector<BYTE>& decryptedData) {
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            return false;
        }

        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptHashData(hHash, (BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptDeriveKey(hProv, CALG_3DES, hHash, 0, &hKey)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptSetKeyParam(hKey, KP_IV, (BYTE*)iv.c_str(), 0)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD mode = CRYPT_MODE_CBC;
        if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD dataLen = (DWORD)encryptedData.size();
        decryptedData = encryptedData;

        if (!CryptDecrypt(hKey, 0, TRUE, 0, decryptedData.data(), &dataLen)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        decryptedData.resize(dataLen);
        decryptedData = removePKCS7Padding(decryptedData);

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        return true;
    }

    static bool decryptAESWithCryptoAPI(const std::vector<BYTE>& encryptedData,
        const std::string& key, const std::string& iv,
        std::vector<BYTE>& decryptedData) {
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                return false;
            }
        }

        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptHashData(hHash, (BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptSetKeyParam(hKey, KP_IV, (BYTE*)iv.c_str(), 0)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD mode = CRYPT_MODE_CBC;
        if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD dataLen = (DWORD)encryptedData.size();
        decryptedData = encryptedData;

        if (!CryptDecrypt(hKey, 0, TRUE, 0, decryptedData.data(), &dataLen)) {
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        decryptedData.resize(dataLen);

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        return true;
    }

public:
    static bool decrypt3DES(const std::string& filePath, const std::string& fileName, const std::string& dstPath) {
        try {
            if (fileName.length() < 21) return false;
            std::string key = fileName.substr(5, 16);

            std::tm creationTime = {};
            std::string fullPath = filePath + fileName;
            if (!getFileCreationTime(fullPath, creationTime)) return false;

            std::string iv = generateTimeString(creationTime);
            if (iv.length() < 8) iv.append(8 - iv.length(), '0');

            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) return false;

            std::vector<BYTE> encryptedData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();

            if (encryptedData.empty()) return false;

            std::vector<BYTE> decryptedData;
            if (!decrypt3DESWithCryptoAPI(encryptedData, key, iv, decryptedData)) return false;

            std::string outputFileName = (fileName.length() > 22) ? fileName.substr(22) : "decrypted_" + fileName;
            std::string outputPath = dstPath + outputFileName;

            CreateDirectoryA(dstPath.c_str(), NULL);

            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) return false;

            outFile.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
            outFile.close();

            return true;

        }
        catch (...) {
            return false;
        }
    }

    static bool decryptAES(const std::string& filePath, const std::string& fileName, const std::string& dstPath) {
        try {
            if (fileName.length() < 20) return false;
            std::string key = fileName.substr(4, 16);

            std::tm creationTime = {};
            std::string fullPath = filePath + fileName;
            if (!getFileCreationTime(fullPath, creationTime)) return false;

            std::string ctime = generateTimeString(creationTime);
            std::string iv = key.substr(4, 8) + ctime;
            if (iv.length() < 16) iv.append(16 - iv.length(), '0');

            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) return false;

            std::vector<BYTE> encryptedData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();

            if (encryptedData.empty()) return false;

            std::vector<BYTE> decryptedData;
            if (!decryptAESWithCryptoAPI(encryptedData, key, iv, decryptedData)) return false;

            std::string outputFileName = (fileName.length() > 21) ? fileName.substr(21) : "decrypted_" + fileName;
            std::string outputPath = dstPath + outputFileName;

            CreateDirectoryA(dstPath.c_str(), NULL);

            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) return false;

            outFile.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
            outFile.close();

            return true;

        }
        catch (...) {
            return false;
        }
    }
};

// 导出的DLL函数
DECRYPT_API bool decrypt3DES(const char* filePath, const char* fileName, const char* dstPath) {
    return FileDecryptor::decrypt3DES(filePath, fileName, dstPath);
}

DECRYPT_API bool decryptAES(const char* filePath, const char* fileName, const char* dstPath) {
    return FileDecryptor::decryptAES(filePath, fileName, dstPath);
}

DECRYPT_API const char* getVersion() {
    return "FileDecryptor DLL v1.0";
}

DECRYPT_API bool initialize() {
    // 可以在这里进行初始化操作
    return true;
}

DECRYPT_API void cleanup() {
    // 可以在这里进行清理操作
}