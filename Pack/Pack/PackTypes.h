#pragma once
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
        TAR_GZ,
        TAR_BZ2,
        TAR
    };
}