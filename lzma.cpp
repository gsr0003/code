#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <stdexcept>

// LZMA库头文件
#include <lzma.h>

namespace fs = std::filesystem;

class LZMACompressor {
private:
    // 缓冲区大小（1MB）
    static constexpr size_t BUFFER_SIZE = 1024 * 1024;

    // 读取文件内容到vector
    static std::vector<uint8_t> readFile(const fs::path& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + filepath.string());
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("读取文件失败: " + filepath.string());
        }
        
        return buffer;
    }
    
    // 写入文件
    static void writeFile(const fs::path& filepath, const std::vector<uint8_t>& data) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("无法创建文件: " + filepath.string());
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    // 流式读取文件
    static std::vector<uint8_t> readFileStream(const fs::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + filepath.string());
        }
        
        std::vector<uint8_t> buffer;
        char temp[BUFFER_SIZE];
        
        while (file.read(temp, BUFFER_SIZE) || file.gcount() > 0) {
            buffer.insert(buffer.end(), temp, temp + file.gcount());
        }
        
        return buffer;
    }

public:
    // LZMA压缩（内存一次性加载）
    static void compressLZMA(const std::string& source_path, const std::string& source_name) {
        fs::path input_path = fs::path(source_path) / source_name;
        fs::path output_path = fs::path(source_path) / ("LZMA_" + source_name + ".xz");
        
        try {
            std::cout << "正在压缩: " << input_path.string() << std::endl;
            
            // 读取原始文件
            auto input_data = readFile(input_path);
            std::cout << "原始文件大小: " << input_data.size() << " 字节" << std::endl;
            
            // 初始化LZMA流
            lzma_stream strm = LZMA_STREAM_INIT;
            lzma_ret ret = lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64);
            
            if (ret != LZMA_OK) {
                throw std::runtime_error("LZMA编码器初始化失败，错误代码: " + std::to_string(ret));
            }
            
            // 准备输出缓冲区（稍大于输入）
            std::vector<uint8_t> output_data(input_data.size() * 2);
            strm.next_in = input_data.data();
            strm.avail_in = input_data.size();
            strm.next_out = output_data.data();
            strm.avail_out = output_data.size();
            
            // 执行压缩
            ret = lzma_code(&strm, LZMA_FINISH);
            if (ret != LZMA_STREAM_END && ret != LZMA_OK) {
                lzma_end(&strm);
                throw std::runtime_error("LZMA压缩失败，错误代码: " + std::to_string(ret));
            }
            
            // 调整输出缓冲区大小
            size_t compressed_size = output_data.size() - strm.avail_out;
            output_data.resize(compressed_size);
            
            // 清理LZMA流
            lzma_end(&strm);
            
            // 写入压缩文件
            writeFile(output_path, output_data);
            
            std::cout << "压缩完成: " << output_path.string() << std::endl;
            std::cout << "压缩后大小: " << compressed_size << " 字节" << std::endl;
            std::cout << "压缩率: " 
                     << (100.0 - (static_cast<double>(compressed_size) / input_data.size() * 100.0))
                     << "%" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "压缩错误: " << e.what() << std::endl;
            throw;
        }
    }
    
    // LZMA解压缩（内存一次性加载）
    static void decompressLZMA(const std::string& source_path, const std::string& source_name) {
        fs::path input_path = fs::path(source_path) / source_name;
        
        // 检查文件名是否以"LZMA_"开头
        std::string output_filename;
        if (source_name.rfind("LZMA_", 0) == 0) {
            // 去除"LZMA_"前缀和".xz"后缀
            output_filename = source_name.substr(5);
            size_t xz_pos = output_filename.rfind(".xz");
            if (xz_pos != std::string::npos) {
                output_filename = output_filename.substr(0, xz_pos);
            }
        } else {
            // 如果不是标准格式，添加"_decompressed"后缀
            output_filename = source_name + "_decompressed";
            size_t xz_pos = output_filename.rfind(".xz");
            if (xz_pos != std::string::npos) {
                output_filename = output_filename.substr(0, xz_pos);
            }
        }
        
        fs::path output_path = fs::path(source_path) / output_filename;
        
        try {
            std::cout << "正在解压: " << input_path.string() << std::endl;
            
            // 读取压缩文件
            auto input_data = readFile(input_path);
            std::cout << "压缩文件大小: " << input_data.size() << " 字节" << std::endl;
            
            // 初始化LZMA流
            lzma_stream strm = LZMA_STREAM_INIT;
            lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
            
            if (ret != LZMA_OK) {
                throw std::runtime_error("LZMA解码器初始化失败，错误代码: " + std::to_string(ret));
            }
            
            // 准备输出缓冲区（初始大小为压缩文件的5倍，通常足够）
            std::vector<uint8_t> output_data(input_data.size() * 5);
            strm.next_in = input_data.data();
            strm.avail_in = input_data.size();
            strm.next_out = output_data.data();
            strm.avail_out = output_data.size();
            
            // 执行解压
            ret = lzma_code(&strm, LZMA_FINISH);
            if (ret != LZMA_STREAM_END && ret != LZMA_OK) {
                lzma_end(&strm);
                throw std::runtime_error("LZMA解压失败，错误代码: " + std::to_string(ret));
            }
            
            // 调整输出缓冲区大小
            size_t decompressed_size = output_data.size() - strm.avail_out;
            output_data.resize(decompressed_size);
            
            // 清理LZMA流
            lzma_end(&strm);
            
            // 写入解压文件
            writeFile(output_path, output_data);
            
            std::cout << "解压完成: " << output_path.string() << std::endl;
            std::cout << "解压后大小: " << decompressed_size << " 字节" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "解压错误: " << e.what() << std::endl;
            throw;
        }
    }
    
    // 流式LZMA压缩（适合大文件）
    static void compressLZMAStream(const std::string& source_path, const std::string& source_name) {
        fs::path input_path = fs::path(source_path) / source_name;
        fs::path output_path = fs::path(source_path) / ("LZMA_" + source_name + ".xz");
        
        try {
            std::cout << "正在流式压缩: " << input_path.string() << std::endl;
            
            std::ifstream input_file(input_path, std::ios::binary);
            if (!input_file.is_open()) {
                throw std::runtime_error("无法打开输入文件: " + input_path.string());
            }
            
            std::ofstream output_file(output_path, std::ios::binary);
            if (!output_file.is_open()) {
                throw std::runtime_error("无法创建输出文件: " + output_path.string());
            }
            
            // 初始化LZMA流
            lzma_stream strm = LZMA_STREAM_INIT;
            lzma_ret ret = lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC64);
            
            if (ret != LZMA_OK) {
                throw std::runtime_error("LZMA编码器初始化失败");
            }
            
            std::vector<uint8_t> in_buf(BUFFER_SIZE);
            std::vector<uint8_t> out_buf(BUFFER_SIZE);
            
            size_t total_in = 0, total_out = 0;
            
            while (!input_file.eof()) {
                input_file.read(reinterpret_cast<char*>(in_buf.data()), BUFFER_SIZE);
                strm.avail_in = input_file.gcount();
                total_in += strm.avail_in;
                strm.next_in = in_buf.data();
                
                do {
                    strm.avail_out = BUFFER_SIZE;
                    strm.next_out = out_buf.data();
                    
                    ret = lzma_code(&strm, input_file.eof() ? LZMA_FINISH : LZMA_RUN);
                    
                    if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
                        lzma_end(&strm);
                        throw std::runtime_error("LZMA压缩失败");
                    }
                    
                    size_t write_size = BUFFER_SIZE - strm.avail_out;
                    total_out += write_size;
                    output_file.write(reinterpret_cast<const char*>(out_buf.data()), write_size);
                    
                } while (strm.avail_out == 0);
            }
            
            lzma_end(&strm);
            
            std::cout << "流式压缩完成: " << output_path.string() << std::endl;
            std::cout << "原始大小: " << total_in << " 字节, "
                      << "压缩后大小: " << total_out << " 字节, "
                      << "压缩率: " << (100.0 - (static_cast<double>(total_out) / total_in * 100.0))
                      << "%" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "流式压缩错误: " << e.what() << std::endl;
            throw;
        }
    }
    
    // 检测是否为LZMA压缩文件
    static bool isLZMAFile(const fs::path& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;
        
        unsigned char header[6];
        file.read(reinterpret_cast<char*>(header), 6);
        
        // LZMA文件通常以特定的魔数开头
        // .xz文件: FD 37 7A 58 5A 00
        return (header[0] == 0xFD && header[1] == 0x37 && 
                header[2] == 0x7A && header[3] == 0x58 && 
                header[4] == 0x5A && header[5] == 0x00);
    }
};

// 使用示例
int main() {
    std::string source_path = "./";
    std::string source_name = "test.txt";
    
    try {
        // 创建测试文件
        std::ofstream test_file(source_path + source_name);
        test_file << "This is a test file for LZMA compression.\n";
        test_file << "LZMA is a compression algorithm providing high compression ratios.\n";
        test_file << "It's commonly used in Linux distributions for package compression.\n";
        test_file.close();
        
        std::cout << "=== LZMA压缩测试 ===\n";
        
        // 方法1：内存一次性压缩
        LZMACompressor::compressLZMA(source_path, source_name);
        
        // 方法2：流式压缩（适合大文件）
        // LZMACompressor::compressLZMAStream(source_path, source_name);
        
        std::cout << "\n=== LZMA解压测试 ===\n";
        
        // 解压文件
        LZMACompressor::decompressLZMA(source_path, "LZMA_" + source_name + ".xz");
        
        // 检测文件是否为LZMA格式
        std::cout << "\n=== 文件检测 ===\n";
        bool is_lzma = LZMACompressor::isLZMAFile(source_path + "LZMA_" + source_name + ".xz");
        std::cout << "检测LZMA文件: " << (is_lzma ? "是" : "否") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "程序出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}