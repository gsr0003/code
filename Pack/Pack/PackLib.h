#pragma once

#ifdef COMPRESSIONLIB_EXPORTS
#define COMPRESSION_API __declspec(dllexport)
#else
#define COMPRESSION_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

namespace CompressionLib
{
    struct CompressionConfig
    {
        int compressionLevel = 6;  // 1-9, 1最快，9最好压缩
        bool preservePath = true;
        bool overwriteExisting = false;
    };

    enum class ArchiveFormat
    {
        ZIP,
        TAR_GZ
    };

    // 初始化库
    extern "C" COMPRESSION_API bool Initialize();

    // 清理资源
    extern "C" COMPRESSION_API void Cleanup();

    // 创建ZIP压缩文件（使用Windows内置压缩）
    extern "C" COMPRESSION_API bool CreateZipArchive(
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config = nullptr
    );

    // 创建TAR.GZ压缩文件（使用Windows内置tar命令）
    extern "C" COMPRESSION_API bool CreateTarGzArchive(
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config = nullptr
    );

    // 通用压缩函数
    extern "C" COMPRESSION_API bool CreateArchive(
        ArchiveFormat format,
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config = nullptr
    );

    // 获取错误信息 - 重命名避免与Windows API冲突
    extern "C" COMPRESSION_API const char* GetCompressionError();

    // 获取压缩库版本 - 重命名避免与Windows API冲突
    extern "C" COMPRESSION_API const char* GetCompressionVersion();
}