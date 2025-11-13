/*#include "pch.h" // VS预编译头
#include "LZMA.h"
#include <fstream>
#include <vector>
#include <string>
#include <lzma.h>

// 全局错误信息
static std::string g_lastError;

#define BUFFER_SIZE 4096

// 内部工具函数
namespace {
    bool readFile(const std::string& filename, std::vector<uint8_t>& data) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            g_lastError = "无法打开文件: " + filename;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            g_lastError = "读取文件失败: " + filename;
            return false;
        }

        return true;
    }

    bool writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            g_lastError = "无法创建文件: " + filename;
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        if (!file.good()) {
            g_lastError = "写入文件失败: " + filename;
            return false;
        }

        return true;
    }

    bool lzmaCompressInternal(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        lzma_stream strm = LZMA_STREAM_INIT;
        lzma_ret ret = lzma_easy_encoder(&strm, 6, LZMA_CHECK_CRC64);

        if (ret != LZMA_OK) {
            g_lastError = "LZMA编码器初始化失败";
            return false;
        }

        strm.next_in = input.data();
        strm.avail_in = input.size();

        std::vector<uint8_t> buffer(BUFFER_SIZE);

        do {
            strm.next_out = buffer.data();
            strm.avail_out = buffer.size();

            ret = lzma_code(&strm, LZMA_FINISH);

            if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
                g_lastError = "LZMA压缩失败，错误代码: " + std::to_string(ret);
                lzma_end(&strm);
                return false;
            }

            size_t write_size = buffer.size() - strm.avail_out;
            output.insert(output.end(), buffer.begin(), buffer.begin() + write_size);

        } while (strm.avail_out == 0);

        lzma_end(&strm);
        return true;
    }

    bool lzmaDecompressInternal(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        lzma_stream strm = LZMA_STREAM_INIT;
        lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);

        if (ret != LZMA_OK) {
            g_lastError = "LZMA解码器初始化失败";
            return false;
        }

        strm.next_in = input.data();
        strm.avail_in = input.size();

        std::vector<uint8_t> buffer(BUFFER_SIZE);

        do {
            strm.next_out = buffer.data();
            strm.avail_out = buffer.size();

            ret = lzma_code(&strm, LZMA_FINISH);

            if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
                g_lastError = "LZMA解压失败，错误代码: " + std::to_string(ret);
                lzma_end(&strm);
                return false;
            }

            size_t write_size = buffer.size() - strm.avail_out;
            output.insert(output.end(), buffer.begin(), buffer.begin() + write_size);

        } while (strm.avail_out == 0);

        lzma_end(&strm);
        return true;
    }
}

// C风格导出函数
extern "C" {
    LZMALIB_API bool LZMA_Compress(const char* source_path, const char* source_name) {
        g_lastError.clear();

        std::string input_file = std::string(source_path) + source_name;
        std::string output_file = std::string(source_path) + "LZMA_" + source_name;

        std::vector<uint8_t> input_data;
        if (!readFile(input_file, input_data)) {
            return false;
        }

        std::vector<uint8_t> compressed_data;
        if (!lzmaCompressInternal(input_data, compressed_data)) {
            return false;
        }

        return writeFile(output_file, compressed_data);
    }

    LZMALIB_API bool LZMA_Decompress(const char* source_path, const char* source_name) {
        g_lastError.clear();

        std::string input_name = source_name;
        if (input_name.find("LZMA_") != 0) {
            g_lastError = "文件名不是LZMA压缩文件格式";
            return false;
        }

        std::string input_file = std::string(source_path) + source_name;
        std::string output_name = input_name.substr(5);
        std::string output_file = std::string(source_path) + output_name;

        std::vector<uint8_t> compressed_data;
        if (!readFile(input_file, compressed_data)) {
            return false;
        }

        std::vector<uint8_t> decompressed_data;
        if (!lzmaDecompressInternal(compressed_data, decompressed_data)) {
            return false;
        }

        return writeFile(output_file, decompressed_data);
    }

    LZMALIB_API const char* LZMA_GetLastError() {
        return g_lastError.c_str();
    }
}

// C++类方法实现
bool LZMAProcessor::compress(const std::string& source_path, const std::string& source_name) {
    return LZMA_Compress(source_path.c_str(), source_name.c_str());
}

bool LZMAProcessor::decompress(const std::string& source_path, const std::string& source_name) {
    return LZMA_Decompress(source_path.c_str(), source_name.c_str());
}

std::string LZMAProcessor::getLastError() {
    return g_lastError;
}
*/
#include "pch.h"
#include "LZMA.h"
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>

static std::string g_lastError;

namespace SimpleCompression {
    // 简单的运行长度编码(RLE)压缩
    bool compressRLE(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        if (input.empty()) return true;

        for (size_t i = 0; i < input.size(); ) {
            uint8_t current = input[i];
            size_t count = 1;

            // 计算连续相同字节的数量
            while (i + count < input.size() && input[i + count] == current && count < 255) {
                count++;
            }

            if (count > 3) {
                // 使用RLE编码
                output.push_back(0xFF); // RLE标记
                output.push_back(current);
                output.push_back(static_cast<uint8_t>(count));
                i += count;
            }
            else {
                // 直接存储原始字节
                for (size_t j = 0; j < count; j++) {
                    if (current == 0xFF) {
                        output.push_back(0xFE); // 转义字符
                    }
                    output.push_back(current);
                }
                i += count;
            }
        }

        return true;
    }

    bool decompressRLE(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        for (size_t i = 0; i < input.size(); ) {
            if (input[i] == 0xFF && i + 2 < input.size()) {
                // RLE编码
                uint8_t value = input[i + 1];
                uint8_t count = input[i + 2];
                output.insert(output.end(), count, value);
                i += 3;
            }
            else if (input[i] == 0xFE && i + 1 < input.size()) {
                // 转义字符
                output.push_back(0xFF);
                i += 2;
            }
            else {
                // 普通字节
                output.push_back(input[i]);
                i++;
            }
        }
        return true;
    }
}

namespace {
    bool readFile(const std::string& filename, std::vector<uint8_t>& data) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            g_lastError = "无法打开文件: " + filename;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            g_lastError = "读取文件失败: " + filename;
            return false;
        }

        return true;
    }

    bool writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            g_lastError = "无法创建文件: " + filename;
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    }
}

extern "C" {
    LZMALIB_API bool LZMA_Compress(const char* source_path, const char* source_name) {
        g_lastError.clear();

        std::string input_file = std::string(source_path) + source_name;
        std::string output_file = std::string(source_path) + "LZMA_" + source_name;

        std::vector<uint8_t> input_data;
        if (!readFile(input_file, input_data)) {
            return false;
        }

        std::vector<uint8_t> compressed_data;
        if (!SimpleCompression::compressRLE(input_data, compressed_data)) {
            g_lastError = "压缩失败";
            return false;
        }

        return writeFile(output_file, compressed_data);
    }

    LZMALIB_API bool LZMA_Decompress(const char* source_path, const char* source_name) {
        g_lastError.clear();

        std::string input_name = source_name;
        if (input_name.find("LZMA_") != 0) {
            g_lastError = "文件名不是LZMA压缩文件格式";
            return false;
        }

        std::string input_file = std::string(source_path) + source_name;
        std::string output_name = input_name.substr(5);
        std::string output_file = std::string(source_path) + output_name;

        std::vector<uint8_t> compressed_data;
        if (!readFile(input_file, compressed_data)) {
            return false;
        }

        std::vector<uint8_t> decompressed_data;
        if (!SimpleCompression::decompressRLE(compressed_data, decompressed_data)) {
            g_lastError = "解压失败";
            return false;
        }

        return writeFile(output_file, decompressed_data);
    }

    LZMALIB_API const char* LZMA_GetLastError() {
        return g_lastError.c_str();
    }
}