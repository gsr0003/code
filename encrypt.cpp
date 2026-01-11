#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <cstring>

class FileEncryptor {
private:
    // 获取当前时间字符串（格式：ddHHMMSS）
    static std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t_now);
        
        std::stringstream ss;
        ss << std::setfill('0') 
           << std::setw(2) << tm.tm_mday
           << std::setw(2) << tm.tm_hour
           << std::setw(2) << tm.tm_min
           << std::setw(2) << tm.tm_sec;
        return ss.str();
    }
    
    // 读取文件内容到vector
    static std::vector<unsigned char> readFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + filepath);
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("读取文件失败: " + filepath);
        }
        
        return buffer;
    }
    
    // 写入文件
    static void writeFile(const std::string& filepath, const std::vector<unsigned char>& data) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("无法创建文件: " + filepath);
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    // PKCS7填充
    static void addPKCS7Padding(std::vector<unsigned char>& data, size_t block_size) {
        size_t padding = block_size - (data.size() % block_size);
        data.insert(data.end(), padding, static_cast<unsigned char>(padding));
    }

public:
    // 3DES加密
    static void encrypt3DES(const std::string& file_path, const std::string& file_name, 
                           const std::string& key) {
        try {
            // 读取文件内容
            std::string full_path = file_path + file_name;
            auto plaintext = readFile(full_path);
            
            // 生成IV（使用当前时间）
            std::string iv_str = getCurrentTime();
            if (iv_str.length() < 8) {
                iv_str.insert(0, 8 - iv_str.length(), '0');
            }
            
            // 准备密钥（3DES需要24字节密钥）
            std::string full_key = key;
            if (full_key.length() < 24) {
                // 如果密钥不足24字节，重复填充
                while (full_key.length() < 24) {
                    full_key += key;
                }
                full_key = full_key.substr(0, 24);
            } else if (full_key.length() > 24) {
                full_key = full_key.substr(0, 24);
            }
            
            // 设置加密上下文
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("无法创建EVP上下文");
            }
            
            // 初始化3DES加密
            if (EVP_EncryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL, 
                                  reinterpret_cast<const unsigned char*>(full_key.c_str()),
                                  reinterpret_cast<const unsigned char*>(iv_str.c_str())) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("3DES加密初始化失败");
            }
            
            // PKCS5/PKCS7填充（在OpenSSL中相同）
            addPKCS7Padding(plaintext, 8); // 3DES块大小为8字节
            
            // 执行加密
            std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
            int out_len = 0, tmp_len = 0;
            
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, 
                                 plaintext.data(), plaintext.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("3DES加密失败");
            }
            
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &tmp_len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("3DES加密完成失败");
            }
            
            out_len += tmp_len;
            ciphertext.resize(out_len);
            
            // 清理上下文
            EVP_CIPHER_CTX_free(ctx);
            
            // 写入加密文件
            std::string output_filename = file_path + "3DES_" + key + "_" + file_name;
            writeFile(output_filename, ciphertext);
            
            std::cout << "3DES加密完成: " << output_filename << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "3DES加密错误: " << e.what() << std::endl;
            throw;
        }
    }
    
    // AES加密
    static void encryptAES(const std::string& file_path, const std::string& file_name,
                          const std::string& key) {
        try {
            // 读取文件内容
            std::string full_path = file_path + file_name;
            auto plaintext = readFile(full_path);
            
            // 生成IV：密钥中间8位 + 当前时间
            std::string key_middle;
            if (key.length() >= 12) {
                key_middle = key.substr(4, 8); // 第5-12位字符
            } else {
                // 如果密钥太短，用整个密钥填充
                key_middle = key;
                while (key_middle.length() < 8) {
                    key_middle += key;
                }
                key_middle = key_middle.substr(0, 8);
            }
            
            std::string iv_str = key_middle + getCurrentTime();
            if (iv_str.length() < 16) {
                iv_str.insert(0, 16 - iv_str.length(), '0');
            } else if (iv_str.length() > 16) {
                iv_str = iv_str.substr(0, 16);
            }
            
            // 准备密钥（AES-128需要16字节密钥）
            std::string full_key = key;
            if (full_key.length() < 16) {
                while (full_key.length() < 16) {
                    full_key += key;
                }
                full_key = full_key.substr(0, 16);
            } else if (full_key.length() > 16) {
                full_key = full_key.substr(0, 16);
            }
            
            // 设置加密上下文
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("无法创建EVP上下文");
            }
            
            // 初始化AES加密
            if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL,
                                  reinterpret_cast<const unsigned char*>(full_key.c_str()),
                                  reinterpret_cast<const unsigned char*>(iv_str.c_str())) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES加密初始化失败");
            }
            
            // PKCS7填充
            addPKCS7Padding(plaintext, AES_BLOCK_SIZE);
            
            // 执行加密
            std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
            int out_len = 0, tmp_len = 0;
            
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                                 plaintext.data(), plaintext.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES加密失败");
            }
            
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &tmp_len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES加密完成失败");
            }
            
            out_len += tmp_len;
            ciphertext.resize(out_len);
            
            // 清理上下文
            EVP_CIPHER_CTX_free(ctx);
            
            // 写入加密文件
            std::string output_filename = file_path + "AES_" + key + "_" + file_name;
            writeFile(output_filename, ciphertext);
            
            std::cout << "AES加密完成: " << output_filename << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "AES加密错误: " << e.what() << std::endl;
            throw;
        }
    }
};

// 使用示例
int main() {
    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    
    std::string file_path = "./";
    std::string file_name = "test.txt";
    std::string key = "MySecretKey12345"; // 密钥长度至少8字符（3DES）或16字符（AES）
    
    try {
        // 3DES加密
        FileEncryptor::encrypt3DES(file_path, file_name, key);
        
        // AES加密
        FileEncryptor::encryptAES(file_path, file_name, key);
        
    } catch (const std::exception& e) {
        std::cerr << "程序出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}