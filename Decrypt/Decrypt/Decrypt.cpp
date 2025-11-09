#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <openssl/des.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <filesystem>
#include <chrono>
#include <iomanip>

class FileDecryptor {
private:
    static std::string getFileCreationTime(const std::string& file_path) {
        namespace fs = std::filesystem;
        auto ftime = fs::last_write_time(file_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm = *std::localtime(&tt);

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_mday
            << std::setw(2) << tm.tm_hour
            << std::setw(2) << tm.tm_min
            << std::setw(2) << tm.tm_sec;
        return oss.str();
    }

    static std::vector<unsigned char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Cannot read file: " + filename);
        }

        return buffer;
    }

    static void writeFile(const std::string& filename, const std::vector<unsigned char>& data) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file: " + filename);
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

public:
    static bool decrypt3DES(const std::string& file_path, const std::string& file_name,
        const std::string& dst_path) {
        try {
            // 提取密钥
            if (file_name.length() < 21) {
                std::cerr << "Filename too short for 3DES decryption" << std::endl;
                return false;
            }
            std::string key = file_name.substr(5, 16);

            // 获取IV
            std::string full_path = file_path + file_name;
            std::string iv_str = getFileCreationTime(full_path);

            // 读取加密文件
            auto encrypted_data = readFile(full_path);

            // 3DES解密
            DES_cblock key1, key2, key3, iv;
            DES_key_schedule ks1, ks2, ks3;

            // 将密钥分成三部分
            memcpy(key1, key.c_str(), 8);
            memcpy(key2, key.c_str() + 4, 8);
            memcpy(key3, key.c_str() + 8, 8);
            memcpy(iv, iv_str.c_str(), 8);

            DES_set_key_unchecked(&key1, &ks1);
            DES_set_key_unchecked(&key2, &ks2);
            DES_set_key_unchecked(&key3, &ks3);

            std::vector<unsigned char> decrypted_data(encrypted_data.size());
            int final_length = 0;

            DES_ede3_cbc_encrypt(encrypted_data.data(), decrypted_data.data(),
                encrypted_data.size(), &ks1, &ks2, &ks3, &iv, DES_DECRYPT);

            // 处理PKCS5填充
            size_t pad_len = decrypted_data.back();
            if (pad_len > 0 && pad_len <= 8) {
                decrypted_data.resize(decrypted_data.size() - pad_len);
            }

            // 写入解密文件
            std::string output_name = file_name.substr(22);
            writeFile(dst_path + output_name, decrypted_data);

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "3DES Decryption failed: " << e.what() << std::endl;
            return false;
        }
    }

    static bool decryptAES(const std::string& file_path, const std::string& file_name,
        const std::string& dst_path) {
        try {
            // 提取密钥
            if (file_name.length() < 20) {
                std::cerr << "Filename too short for AES decryption" << std::endl;
                return false;
            }
            std::string key = file_name.substr(4, 16);

            // 获取IV
            std::string full_path = file_path + file_name;
            std::string ctime = getFileCreationTime(full_path);
            std::string iv = key.substr(4, 8) + ctime;

            // 读取加密文件
            auto encrypted_data = readFile(full_path);

            // AES解密
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("Failed to create EVP context");
            }

            if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL,
                reinterpret_cast<const unsigned char*>(key.c_str()),
                reinterpret_cast<const unsigned char*>(iv.c_str())) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("Failed to initialize AES decryption");
            }

            std::vector<unsigned char> decrypted_data(encrypted_data.size() + AES_BLOCK_SIZE);
            int out_len1 = 0, out_len2 = 0;

            if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &out_len1,
                encrypted_data.data(), encrypted_data.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES decryption update failed");
            }

            if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + out_len1, &out_len2) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                throw std::runtime_error("AES decryption final failed");
            }

            EVP_CIPHER_CTX_free(ctx);

            decrypted_data.resize(out_len1 + out_len2);

            // 写入解密文件
            std::string output_name = file_name.substr(21);
            writeFile(dst_path + output_name, decrypted_data);

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "AES Decryption failed: " << e.what() << std::endl;
            return false;
        }
    }
};

// 使用示例
int main() {
    // 3DES解密示例
    FileDecryptor::decrypt3DES("encrypted/", "des_key123456789012_original.txt", "decrypted/");

    // AES解密示例  
    FileDecryptor::decryptAES("encrypted/", "aes_key123456789012_original.txt", "decrypted/");

    return 0;
}