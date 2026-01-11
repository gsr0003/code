#pragma once

#ifdef MINIMALARCHIVE_EXPORTS
#define MINIMAL_API __declspec(dllexport)
#else
#define MINIMAL_API __declspec(dllimport)
#endif

#include <string>

// 前向声明
class IMinimalExtractor;

extern "C" {
    // 创建解包器实例
    MINIMAL_API void* CreateMinimalExtractor(const char* basePath);

    // 销毁解包器实例
    MINIMAL_API void DestroyMinimalExtractor(void* extractor);

    // 通用解包函数（自动检测格式）
    MINIMAL_API bool UnpackArchive(void* extractor, const char* filename);

    // 设置解压目录
    MINIMAL_API void SetExtractionPath(void* extractor, const char* path);

    // 获取错误信息
    MINIMAL_API const char* GetLastErrorMessage(void* extractor);

    // 检查是否支持某种格式
    MINIMAL_API bool IsFormatSupported(const char* filename);
}

// C++ 类接口
class IMinimalExtractor {
public:
    virtual ~IMinimalExtractor() = default;
    virtual bool unpackFile(const std::string& filename) = 0;
    virtual void setExtractionPath(const std::string& path) = 0;
    virtual std::string getLastError() const = 0;
    virtual bool isFormatSupported(const std::string& filename) = 0;
};

// 创建C++接口实例
MINIMAL_API IMinimalExtractor* CreateMinimalExtractorInterface(const std::string& basePath);
MINIMAL_API void DestroyMinimalExtractorInterface(IMinimalExtractor* extractor);