#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

class FileDecryptor {
private:
    // 获取文件创建时间的日时分秒字符串
    static std::string getFileTimeString(const std::string& file_path) {
        try {
            auto ftime = fs::last_write_time(file_path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
            std::tm tm = *std::localtime(&tt);
            
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(2) << tm.tm_mday
                << std::setw(2) << tm.tm_hour
                << std::setw(2) << tm.tm_min
                << std::setw(2) << tm.tm_sec;
            return oss.str();
        } catch (...) {
            throw std::runtime_error("无法获取文件时间");
        }
    }

    // 读取文件内容到vector
    static std::vector<unsigned char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("无法打开文件: " + filename);
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("读取文件失败: " + filename);
        }
        
        return buffer;
    }

    // 写入数据到文件
    static void writeFile(const std::string& filename, const std::vector<unsigned char>& data) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("无法创建文件: " + filename);
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

public:
    // 3DES解密
    static void decrypt3DES(const std::string& file_path, const std::string& file_name, const std::string& dst_path) {
        try {
            // 从文件名提取密钥
            if (file_name.length() < 21) {
                throw std::runtime_error("文件名格式错误");
            }
            std::string key = file_name.substr(5, 16);
            
            // 获取IV
            std::string full_path = file_path + file_name;
            std::string iv = getFileTimeString(full_path);
            
            // 读取加密文件
            auto encrypted_data = readFile(full_path);
            
            // 3DES解密
            DES_cblock key1, key2, key3, iv_block;
            DES_key_schedule ks1, ks2, ks3;
            
            // 将密钥分成三部分
            memcpy(key1, key.c_str(), 8);
            memcpy(key2, key.c_str() + 4, 8);
            memcpy(key3, key.c_str() + 8, 8);
            memcpy(iv_block, iv.c_str(), 8);
            
            DES_set_key_unchecked(&key1, &ks1);
            DES_set_key_unchecked(&key2, &ks2);
            DES_set_key_unchecked(&key3, &ks3);
            
            std::vector<unsigned char> decrypted_data(encrypted_data.size());
            int final_length = 0;
            
            DES_ede3_cbc_encrypt(encrypted_data.data(), decrypted_data.data(), 
                               encrypted_data.size(), &ks1, &ks2, &ks3, &iv_block, DES_DECRYPT);
            
            // 处理PKCS5/7填充
            size_t pad_length = decrypted_data.back();
            if (pad_length > 0 && pad_length <= 8) {
                decrypted_data.resize(decrypted_data.size() - pad_length);
            }
            
            // 写入解密文件
            std::string output_filename = file_name.length() >= 22 ? file_name.substr(22) : file_name;
            writeFile(dst_path + output_filename, decrypted_data);
            
            std::cout << "3DES解密完成: " << output_filename << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "3DES解密错误: " << e.what() << std::endl;
            throw;
        }
    }

    // AES解密
    static void decryptAES(const std::string& file_path, const std::string& file_name, const std::string& dst_path) {
        try {
            // 从文件名提取密钥
            if (file_name.length() < 20) {
                throw std::runtime_error("文件名格式错误");
            }
            std::string key = file_name.substr(4, 16);
            
            // 获取IV
            std::string full_path = file_path + file_name;
            std::string time_str = getFileTimeString(full_path);
            std::string iv = key.substr(4, 8) + time_str;
            
            // 读取加密文件
            auto encrypted_data = readFile(full_path);
            
            // AES解密
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("无法创建EVP上下文");
            }
            
            if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, 
                                 reinterpret_cast<const unsigned char*>(key.c_str()),
                                 reinterpret_cast<const unsigned char*>(iv.c_str())) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES解密初始化失败");
            }
            
            std::vector<unsigned char> decrypted_data(encrypted_data.size() + AES_BLOCK_SIZE);
            int out_len1 = 0, out_len2 = 0;
            
            if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &out_len1,
                                encrypted_data.data(), encrypted_data.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES解密更新失败");
            }
            
            if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + out_len1, &out_len2) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES解密结束失败");
            }
            
            EVP_CIPHER_CTX_free(ctx);
            
            decrypted_data.resize(out_len1 + out_len2);
            
            // 写入解密文件
            std::string output_filename = file_name.length() >= 21 ? file_name.substr(21) : file_name;
            writeFile(dst_path + output_filename, decrypted_data);
            
            std::cout << "AES解密完成: " << output_filename << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "AES解密错误: " << e.what() << std::endl;
            throw;
        }
    }
};

// 使用示例
int main() {
    try {
        // 示例用法
        std::string file_path = "./encrypted/";
        std::string dst_path = "./decrypted/";
        
        // 创建输出目录
        fs::create_directories(dst_path);
        
        // 假设有加密文件
        // FileDecryptor::decrypt3DES(file_path, "des_key123456789012_filename.txt", dst_path);
        // FileDecryptor::decryptAES(file_path, "aes_key1234567890123_filename.txt", dst_path);
        
        std::cout << "解密功能就绪" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}