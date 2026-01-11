#pragma once

#ifdef LZMALIB_EXPORTS
#define LZMALIB_API __declspec(dllexport)
#else
#define LZMALIB_API __declspec(dllimport)
#endif

#include <string>

extern "C" {
    // 压缩文件
    LZMALIB_API bool LZMA_Compress(const char* source_path, const char* source_name);

    // 解压文件
    LZMALIB_API bool LZMA_Decompress(const char* source_path, const char* source_name);

    // 获取错误信息
    LZMALIB_API const char* LZMA_GetLastError();
}

// C++ 类接口（可选）
class LZMALIB_API LZMAProcessor {
public:
    static bool compress(const std::string& source_path, const std::string& source_name);
    static bool decompress(const std::string& source_path, const std::string& source_name);
    static std::string getLastError();
};