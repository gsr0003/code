//#include "Decrypt.h"
#include "pch.h"
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

class FileDecryptor {
private:
    // 获取文件创建时间
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

    // 生成时间字符串（日+时+分+秒）
    static std::string generateTimeString(const std::tm& timeinfo) {
        std::stringstream ss;
        ss << std::setfill('0') 
           << std::setw(2) << timeinfo.tm_mday
           << std::setw(2) << timeinfo.tm_hour
           << std::setw(2) << timeinfo.tm_min
           << std::setw(2) << timeinfo.tm_sec;
        return ss.str();
    }

    // PKCS7去填充
    static std::vector<BYTE> removePKCS7Padding(const std::vector<BYTE>& data) {
        if (data.empty()) return data;
        
        BYTE padValue = data[data.size() - 1];
        if (padValue > data.size()) return data;
        
        // 验证填充是否正确
        for (size_t i = data.size() - padValue; i < data.size(); ++i) {
            if (data[i] != padValue) return data;
        }
        
        return std::vector<BYTE>(data.begin(), data.end() - padValue);
    }

    // 使用CryptoAPI进行3DES解密
    static bool decrypt3DESWithCryptoAPI(const std::vector<BYTE>& encryptedData,
                                        const std::string& key, const std::string& iv,
                                        std::vector<BYTE>& decryptedData) {
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;
        
        // 获取CSP句柄
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            std::cerr << "CryptAcquireContext失败: " << GetLastError() << std::endl;
            return false;
        }
        
        // 创建哈希对象
        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            std::cerr << "CryptCreateHash失败: " << GetLastError() << std::endl;
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 哈希密钥数据
        if (!CryptHashData(hHash, (BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
            std::cerr << "CryptHashData失败: " << GetLastError() << std::endl;
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 从哈希派生密钥
        if (!CryptDeriveKey(hProv, CALG_3DES, hHash, 0, &hKey)) {
            std::cerr << "CryptDeriveKey失败: " << GetLastError() << std::endl;
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 设置IV
        if (!CryptSetKeyParam(hKey, KP_IV, (BYTE*)iv.c_str(), 0)) {
            std::cerr << "CryptSetKeyParam失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 设置加密模式为CBC
        DWORD mode = CRYPT_MODE_CBC;
        if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0)) {
            std::cerr << "设置CBC模式失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 解密数据
        DWORD dataLen = (DWORD)encryptedData.size();
        decryptedData = encryptedData;
        
        if (!CryptDecrypt(hKey, 0, TRUE, 0, decryptedData.data(), &dataLen)) {
            std::cerr << "CryptDecrypt失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 调整大小并去除填充
        decryptedData.resize(dataLen);
        decryptedData = removePKCS7Padding(decryptedData);
        
        // 清理资源
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        
        return true;
    }

    // 使用CryptoAPI进行AES解密
    static bool decryptAESWithCryptoAPI(const std::vector<BYTE>& encryptedData,
                                       const std::string& key, const std::string& iv,
                                       std::vector<BYTE>& decryptedData) {
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;
        
        // 获取CSP句柄
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            // 如果AES provider失败，尝试使用全功能provider
            if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                std::cerr << "CryptAcquireContext失败: " << GetLastError() << std::endl;
                return false;
            }
        }
        
        // 创建哈希对象
        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            std::cerr << "CryptCreateHash失败: " << GetLastError() << std::endl;
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 哈希密钥数据
        if (!CryptHashData(hHash, (BYTE*)key.c_str(), (DWORD)key.length(), 0)) {
            std::cerr << "CryptHashData失败: " << GetLastError() << std::endl;
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 从哈希派生密钥
        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) {
            std::cerr << "CryptDeriveKey失败: " << GetLastError() << std::endl;
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 设置IV
        if (!CryptSetKeyParam(hKey, KP_IV, (BYTE*)iv.c_str(), 0)) {
            std::cerr << "CryptSetKeyParam失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 设置加密模式为CBC
        DWORD mode = CRYPT_MODE_CBC;
        if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0)) {
            std::cerr << "设置CBC模式失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 解密数据
        DWORD dataLen = (DWORD)encryptedData.size();
        decryptedData = encryptedData;
        
        if (!CryptDecrypt(hKey, 0, TRUE, 0, decryptedData.data(), &dataLen)) {
            std::cerr << "CryptDecrypt失败: " << GetLastError() << std::endl;
            CryptDestroyKey(hKey);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }
        
        // 调整大小
        decryptedData.resize(dataLen);
        
        // 清理资源
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        
        return true;
    }

public:
    // 3DES解密
    static bool decrypt3DES(const std::string& filePath, const std::string& fileName, 
                           const std::string& dstPath) {
        try {
            // 从文件名提取密钥
            if (fileName.length() < 21) {
                std::cerr << "文件名太短，无法提取密钥: " << fileName << std::endl;
                return false;
            }
            std::string key = fileName.substr(5, 16);
            
            // 获取文件创建时间
            std::tm creationTime = {};
            std::string fullPath = filePath + fileName;
            if (!getFileCreationTime(fullPath, creationTime)) {
                std::cerr << "无法获取文件创建时间: " << fullPath << std::endl;
                return false;
            }
            
            // 生成IV
            std::string iv = generateTimeString(creationTime);
            if (iv.length() < 8) {
                iv.append(8 - iv.length(), '0'); // 填充到8字节
            }
            
            std::cout << "3DES解密 - 密钥: " << key << " (长度: " << key.length() 
                      << "), IV: " << iv << " (长度: " << iv.length() << ")" << std::endl;
            
            // 读取加密文件 - 使用c_str()转换为C字符串
            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) {
                std::cerr << "无法打开输入文件: " << inputFilePath << std::endl;
                return false;
            }
            
            std::vector<BYTE> encryptedData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();
            
            if (encryptedData.empty()) {
                std::cerr << "加密文件为空" << std::endl;
                return false;
            }

            std::cout << "读取加密数据大小: " << encryptedData.size() << " 字节" << std::endl;

            // 使用CryptoAPI进行3DES解密
            std::vector<BYTE> decryptedData;
            if (!decrypt3DESWithCryptoAPI(encryptedData, key, iv, decryptedData)) {
                std::cerr << "3DES解密过程失败" << std::endl;
                return false;
            }
            
            // 写入输出文件
            std::string outputFileName = (fileName.length() > 22) ? fileName.substr(22) : "decrypted_" + fileName;
            std::string outputPath = dstPath + outputFileName;
            
            // 确保输出目录存在
            CreateDirectoryA(dstPath.c_str(), NULL);
            
            // 使用c_str()转换为C字符串
            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) {
                std::cerr << "无法创建输出文件: " << outputPath << std::endl;
                return false;
            }
            
            outFile.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
            outFile.close();
            
            std::cout << "3DES解密完成: " << outputPath << " (" << decryptedData.size() << " 字节)" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "3DES解密错误: " << e.what() << std::endl;
            return false;
        }
    }

    // AES解密
    static bool decryptAES(const std::string& filePath, const std::string& fileName, 
                          const std::string& dstPath) {
        try {
            // 从文件名提取密钥
            if (fileName.length() < 20) {
                std::cerr << "文件名太短，无法提取密钥: " << fileName << std::endl;
                return false;
            }
            std::string key = fileName.substr(4, 16);
            
            // 获取文件创建时间
            std::tm creationTime = {};
            std::string fullPath = filePath + fileName;
            if (!getFileCreationTime(fullPath, creationTime)) {
                std::cerr << "无法获取文件创建时间: " << fullPath << std::endl;
                return false;
            }
            
            // 生成IV
            std::string ctime = generateTimeString(creationTime);
            std::string iv = key.substr(4, 8) + ctime;
            if (iv.length() < 16) {
                iv.append(16 - iv.length(), '0'); // 填充到16字节
            }
            
            std::cout << "AES解密 - 密钥: " << key << " (长度: " << key.length() 
                      << "), IV: " << iv << " (长度: " << iv.length() << ")" << std::endl;
            
            // 读取加密文件 - 使用c_str()转换为C字符串
            std::string inputFilePath = filePath + fileName;
            std::ifstream inFile(inputFilePath.c_str(), std::ios::binary);
            if (!inFile) {
                std::cerr << "无法打开输入文件: " << inputFilePath << std::endl;
                return false;
            }
            
            std::vector<BYTE> encryptedData(
                (std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>()
            );
            inFile.close();
            
            if (encryptedData.empty()) {
                std::cerr << "加密文件为空" << std::endl;
                return false;
            }

            std::cout << "读取加密数据大小: " << encryptedData.size() << " 字节" << std::endl;

            // 使用CryptoAPI进行AES解密
            std::vector<BYTE> decryptedData;
            if (!decryptAESWithCryptoAPI(encryptedData, key, iv, decryptedData)) {
                std::cerr << "AES解密过程失败" << std::endl;
                return false;
            }
            
            // 写入输出文件
            std::string outputFileName = (fileName.length() > 21) ? fileName.substr(21) : "decrypted_" + fileName;
            std::string outputPath = dstPath + outputFileName;
            
            // 确保输出目录存在
            CreateDirectoryA(dstPath.c_str(), NULL);
            
            // 使用c_str()转换为C字符串
            std::ofstream outFile(outputPath.c_str(), std::ios::binary);
            if (!outFile) {
                std::cerr << "无法创建输出文件: " << outputPath << std::endl;
                return false;
            }
            
            outFile.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
            outFile.close();
            
            std::cout << "AES解密完成: " << outputPath << " (" << decryptedData.size() << " 字节)" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "AES解密错误: " << e.what() << std::endl;
            return false;
        }
    }
};

// 测试函数
void createTestFiles() {
    // 创建测试目录
    CreateDirectoryA("./encrypted", NULL);
    CreateDirectoryA("./decrypted", NULL);
    
    // 创建测试加密文件 - 使用c_str()转换为C字符串
    std::ofstream testFile1("./encrypted/prefix_16bytekey123456_suffix.dat", std::ios::binary);
    std::ofstream testFile2("./encrypted/pre_16bytekey1234567_suffix.dat", std::ios::binary);
    
    if (testFile1) {
        testFile1 << "This is a test encrypted file for 3DES";
        testFile1.close();
        std::cout << "创建3DES测试文件: prefix_16bytekey123456_suffix.dat" << std::endl;
    }
    if (testFile2) {
        testFile2 << "This is a test encrypted file for AES";
        testFile2.close();
        std::cout << "创建AES测试文件: pre_16bytekey1234567_suffix.dat" << std::endl;
    }
}

// 使用示例
int main() {
    std::cout << "开始解密测试..." << std::endl;
    
    // 创建测试文件
    createTestFiles();
    
    std::string filePath = "./encrypted/";
    std::string dstPath = "./decrypted/";
    
    // 测试文件名（根据Python代码的命名规则）
    std::string desFileName = "prefix_16bytekey123456_suffix.dat";
    std::string aesFileName = "pre_16bytekey1234567_suffix.dat";
    
    // 3DES解密
    std::cout << "\n=== 开始3DES解密 ===" << std::endl;
    if (FileDecryptor::decrypt3DES(filePath, desFileName, dstPath)) {
        std::cout << "3DES解密成功!" << std::endl;
    } else {
        std::cout << "3DES解密失败!" << std::endl;
    }
    
    // AES解密
    std::cout << "\n=== 开始AES解密 ===" << std::endl;
    if (FileDecryptor::decryptAES(filePath, aesFileName, dstPath)) {
        std::cout << "AES解密成功!" << std::endl;
    } else {
        std::cout << "AES解密失败!" << std::endl;
    }
    
    std::cout << "测试完成，按任意键退出..." << std::endl;
    std::cin.get();
    
    return 0;
}
