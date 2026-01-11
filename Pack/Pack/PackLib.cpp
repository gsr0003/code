#include "pch.h"
#include "PackLib.h"
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>

namespace CompressionLib
{
    namespace Internal
    {
        std::string g_lastError;
        bool g_initialized = false;

        void SetLastError(const std::string& error)
        {
            g_lastError = error;
        }

        const char* GetLastError()
        {
            return g_lastError.c_str();
        }

        // 检查文件或目录是否存在
        bool PathExists(const std::string& path)
        {
            DWORD attrib = GetFileAttributesA(path.c_str());
            return (attrib != INVALID_FILE_ATTRIBUTES);
        }

        // 检查是否是目录
        bool IsDirectory(const std::string& path)
        {
            DWORD attrib = GetFileAttributesA(path.c_str());
            return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
        }

        // 创建目录
        bool CreateDirectoryRecursive(const std::string& path)
        {
            if (PathExists(path))
                return true;

            // 找到上级目录
            size_t pos = path.find_last_of("\\/");
            if (pos != std::string::npos)
            {
                std::string parent = path.substr(0, pos);
                if (!CreateDirectoryRecursive(parent))
                    return false;
            }

            return CreateDirectoryA(path.c_str(), nullptr) != 0;
        }

        // 标准化路径
        std::string NormalizePath(const std::string& path)
        {
            char buffer[MAX_PATH] = { 0 };
            if (GetFullPathNameA(path.c_str(), MAX_PATH, buffer, nullptr) == 0)
                return path;
            return buffer;
        }

        // 连接路径
        std::string JoinPath(const std::string& path1, const std::string& path2)
        {
            if (path1.empty()) return path2;
            if (path2.empty()) return path1;

            char lastChar = path1[path1.length() - 1];
            if (lastChar == '\\' || lastChar == '/')
                return path1 + path2;
            else
                return path1 + "\\" + path2;
        }

        // 获取文件名
        std::string GetFileName(const std::string& path)
        {
            size_t pos = path.find_last_of("\\/");
            if (pos == std::string::npos)
                return path;
            return path.substr(pos + 1);
        }

        // 简化的ZIP文件创建（纯C++实现）
        bool CreateSimpleZip(const std::string& zipPath, const std::vector<std::string>& filesToAdd)
        {
            // 创建空的ZIP文件
            std::ofstream zipFile(zipPath, std::ios::binary);
            if (!zipFile)
            {
                SetLastError("Failed to create ZIP file: " + zipPath);
                return false;
            }

            std::vector<uint32_t> fileOffsets;
            std::vector<uint32_t> fileSizes;

            // 添加每个文件到ZIP
            for (const auto& file : filesToAdd)
            {
                if (!PathExists(file))
                    continue;

                uint32_t fileOffset = static_cast<uint32_t>(zipFile.tellp());

                if (IsDirectory(file))
                {
                    // 目录条目
                    std::string dirName = GetFileName(file) + "/";

                    // 本地文件头
                    const char* header = "PK\x03\x04"; // Local file header signature
                    zipFile.write(header, 4);

                    uint16_t version = 20;
                    uint16_t flags = 0;
                    uint16_t compression = 0; // Store
                    uint16_t modTime = 0;
                    uint16_t modDate = 0;
                    uint32_t crc32 = 0;
                    uint32_t compressedSize = 0;
                    uint32_t uncompressedSize = 0;
                    uint16_t nameLength = static_cast<uint16_t>(dirName.length());
                    uint16_t extraLength = 0;

                    zipFile.write(reinterpret_cast<const char*>(&version), 2);
                    zipFile.write(reinterpret_cast<const char*>(&flags), 2);
                    zipFile.write(reinterpret_cast<const char*>(&compression), 2);
                    zipFile.write(reinterpret_cast<const char*>(&modTime), 2);
                    zipFile.write(reinterpret_cast<const char*>(&modDate), 2);
                    zipFile.write(reinterpret_cast<const char*>(&crc32), 4);
                    zipFile.write(reinterpret_cast<const char*>(&compressedSize), 4);
                    zipFile.write(reinterpret_cast<const char*>(&uncompressedSize), 4);
                    zipFile.write(reinterpret_cast<const char*>(&nameLength), 2);
                    zipFile.write(reinterpret_cast<const char*>(&extraLength), 2);
                    zipFile.write(dirName.c_str(), nameLength);

                    fileOffsets.push_back(fileOffset);
                    fileSizes.push_back(0);
                }
                else
                {
                    // 文件条目
                    std::ifstream inputFile(file, std::ios::binary);
                    if (!inputFile)
                        continue;

                    // 读取文件内容
                    inputFile.seekg(0, std::ios::end);
                    size_t fileSize = inputFile.tellg();
                    inputFile.seekg(0, std::ios::beg);

                    std::vector<char> fileData(fileSize);
                    inputFile.read(fileData.data(), fileSize);

                    std::string fileName = GetFileName(file);

                    // 本地文件头
                    const char* header = "PK\x03\x04"; // Local file header signature
                    zipFile.write(header, 4);

                    uint16_t version = 20;
                    uint16_t flags = 0;
                    uint16_t compression = 0; // Store (no compression)
                    uint16_t modTime = 0;
                    uint16_t modDate = 0;
                    uint32_t crc32 = 0; // 简化处理，实际应该计算CRC32
                    uint32_t compressedSize = static_cast<uint32_t>(fileSize);
                    uint32_t uncompressedSize = static_cast<uint32_t>(fileSize);
                    uint16_t nameLength = static_cast<uint16_t>(fileName.length());
                    uint16_t extraLength = 0;

                    zipFile.write(reinterpret_cast<const char*>(&version), 2);
                    zipFile.write(reinterpret_cast<const char*>(&flags), 2);
                    zipFile.write(reinterpret_cast<const char*>(&compression), 2);
                    zipFile.write(reinterpret_cast<const char*>(&modTime), 2);
                    zipFile.write(reinterpret_cast<const char*>(&modDate), 2);
                    zipFile.write(reinterpret_cast<const char*>(&crc32), 4);
                    zipFile.write(reinterpret_cast<const char*>(&compressedSize), 4);
                    zipFile.write(reinterpret_cast<const char*>(&uncompressedSize), 4);
                    zipFile.write(reinterpret_cast<const char*>(&nameLength), 2);
                    zipFile.write(reinterpret_cast<const char*>(&extraLength), 2);
                    zipFile.write(fileName.c_str(), nameLength);
                    zipFile.write(fileData.data(), fileSize);

                    fileOffsets.push_back(fileOffset);
                    fileSizes.push_back(static_cast<uint32_t>(fileSize));
                }
            }

            // 中央目录记录
            uint32_t centralDirOffset = static_cast<uint32_t>(zipFile.tellp());
            for (size_t i = 0; i < filesToAdd.size(); ++i)
            {
                if (!PathExists(filesToAdd[i])) continue;

                const char* centralHeader = "PK\x01\x02"; // Central directory file header signature
                zipFile.write(centralHeader, 4);

                std::string fileName = IsDirectory(filesToAdd[i]) ?
                    GetFileName(filesToAdd[i]) + "/" : GetFileName(filesToAdd[i]);

                uint16_t versionMadeBy = 20;
                uint16_t versionNeeded = 20;
                uint16_t flags = 0;
                uint16_t compression = 0;
                uint16_t modTime = 0;
                uint16_t modDate = 0;
                uint32_t crc32 = 0;
                uint32_t compressedSize = fileSizes[i];
                uint32_t uncompressedSize = fileSizes[i];
                uint16_t nameLength = static_cast<uint16_t>(fileName.length());
                uint16_t extraLength = 0;
                uint16_t commentLength = 0;
                uint16_t diskNumber = 0;
                uint16_t internalAttributes = 0;
                uint32_t externalAttributes = 0;
                uint32_t relativeOffset = fileOffsets[i];

                zipFile.write(reinterpret_cast<const char*>(&versionMadeBy), 2);
                zipFile.write(reinterpret_cast<const char*>(&versionNeeded), 2);
                zipFile.write(reinterpret_cast<const char*>(&flags), 2);
                zipFile.write(reinterpret_cast<const char*>(&compression), 2);
                zipFile.write(reinterpret_cast<const char*>(&modTime), 2);
                zipFile.write(reinterpret_cast<const char*>(&modDate), 2);
                zipFile.write(reinterpret_cast<const char*>(&crc32), 4);
                zipFile.write(reinterpret_cast<const char*>(&compressedSize), 4);
                zipFile.write(reinterpret_cast<const char*>(&uncompressedSize), 4);
                zipFile.write(reinterpret_cast<const char*>(&nameLength), 2);
                zipFile.write(reinterpret_cast<const char*>(&extraLength), 2);
                zipFile.write(reinterpret_cast<const char*>(&commentLength), 2);
                zipFile.write(reinterpret_cast<const char*>(&diskNumber), 2);
                zipFile.write(reinterpret_cast<const char*>(&internalAttributes), 2);
                zipFile.write(reinterpret_cast<const char*>(&externalAttributes), 4);
                zipFile.write(reinterpret_cast<const char*>(&relativeOffset), 4);
                zipFile.write(fileName.c_str(), nameLength);
            }

            uint32_t centralDirSize = static_cast<uint32_t>(zipFile.tellp()) - centralDirOffset;

            // 中央目录结尾
            const char* centralEnd = "PK\x05\x06"; // Central directory end signature
            zipFile.write(centralEnd, 4);

            uint16_t diskNumber = 0;
            uint16_t centralStartDisk = 0;
            uint16_t centralEntries = static_cast<uint16_t>(filesToAdd.size());
            uint16_t totalEntries = static_cast<uint16_t>(filesToAdd.size());
            uint32_t centralSize = centralDirSize;
            uint32_t centralOffset = centralDirOffset;
            uint16_t commentLength = 0;

            zipFile.write(reinterpret_cast<const char*>(&diskNumber), 2);
            zipFile.write(reinterpret_cast<const char*>(&centralStartDisk), 2);
            zipFile.write(reinterpret_cast<const char*>(&centralEntries), 2);
            zipFile.write(reinterpret_cast<const char*>(&totalEntries), 2);
            zipFile.write(reinterpret_cast<const char*>(&centralSize), 4);
            zipFile.write(reinterpret_cast<const char*>(&centralOffset), 4);
            zipFile.write(reinterpret_cast<const char*>(&commentLength), 2);

            zipFile.close();
            return true;
        }

        // 使用Windows tar命令创建tar.gz
        bool CreateTarGzWithCommand(const std::string& sourcePath, const std::vector<std::string>& files,
            const std::string& outputPath)
        {
            // 保存当前目录
            char currentDir[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, currentDir);

            // 切换到源目录
            if (!SetCurrentDirectoryA(sourcePath.c_str()))
            {
                SetLastError("Failed to change directory to: " + sourcePath);
                return false;
            }

            // 构建tar命令
            std::stringstream cmd;
            cmd << "tar -czf \"" << outputPath << "\"";

            // 添加文件到命令
            for (const auto& file : files)
            {
                cmd << " \"" << file << "\"";
            }

            std::string command = cmd.str();

            // 执行tar命令
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            BOOL success = CreateProcessA(
                nullptr,
                const_cast<LPSTR>(command.c_str()),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &si,
                &pi
            );

            if (!success)
            {
                SetCurrentDirectoryA(currentDir);
                SetLastError("Failed to execute tar command: " + command);
                return false;
            }

            // 等待命令完成
            WaitForSingleObject(pi.hProcess, 30000);

            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            SetCurrentDirectoryA(currentDir);

            return exitCode == 0;
        }

        // 使用PowerShell创建ZIP（Windows 8+）
        bool CreateZipWithPowerShell(const std::string& sourcePath, const std::vector<std::string>& files,
            const std::string& outputPath)
        {
            // 构建PowerShell命令
            std::stringstream cmd;
            cmd << "powershell -Command \"Compress-Archive -Path '";

            for (size_t i = 0; i < files.size(); ++i)
            {
                if (i > 0) cmd << "', '";
                cmd << Internal::JoinPath(sourcePath, files[i]);
            }

            cmd << "' -DestinationPath '" << outputPath << "' -Force\"";

            std::string command = cmd.str();

            // 执行PowerShell命令
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            BOOL success = CreateProcessA(
                nullptr,
                const_cast<LPSTR>(command.c_str()),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &si,
                &pi
            );

            if (!success)
            {
                SetLastError("Failed to execute PowerShell command");
                return false;
            }

            WaitForSingleObject(pi.hProcess, 30000);

            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return exitCode == 0;
        }
    }

    // 导出函数实现
    extern "C" COMPRESSION_API bool Initialize()
    {
        Internal::g_initialized = true;
        Internal::SetLastError("");
        return true;
    }

    extern "C" COMPRESSION_API void Cleanup()
    {
        Internal::g_initialized = false;
        Internal::SetLastError("");
    }

    extern "C" COMPRESSION_API bool CreateZipArchive(
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config)
    {
        if (!Internal::g_initialized)
        {
            Internal::SetLastError("Library not initialized. Call Initialize() first.");
            return false;
        }

        try
        {
            std::string srcPath = Internal::NormalizePath(sourcePath);
            std::string dstPath = Internal::NormalizePath(destinationPath);
            std::string fullArchivePath = Internal::JoinPath(dstPath, archiveName);

            // 确保目标目录存在
            if (!Internal::CreateDirectoryRecursive(dstPath))
            {
                Internal::SetLastError("Failed to create destination directory: " + dstPath);
                return false;
            }

            // 构建完整文件路径列表
            std::vector<std::string> fullPaths;
            for (int i = 0; i < fileCount; ++i)
            {
                std::string fullPath = Internal::JoinPath(srcPath, fileList[i]);
                if (!Internal::PathExists(fullPath))
                {
                    Internal::SetLastError("File not found: " + fullPath);
                    return false;
                }
                fullPaths.push_back(fullPath);
            }

            // 首先尝试PowerShell方法（Windows 8+）
            std::vector<std::string> relativeFiles;
            for (int i = 0; i < fileCount; ++i)
            {
                relativeFiles.push_back(fileList[i]);
            }

            if (Internal::CreateZipWithPowerShell(srcPath, relativeFiles, fullArchivePath))
            {
                return true;
            }

            // 回退到简化ZIP实现
            return Internal::CreateSimpleZip(fullArchivePath, fullPaths);
        }
        catch (const std::exception& e)
        {
            Internal::SetLastError("Exception in CreateZipArchive: " + std::string(e.what()));
            return false;
        }
    }

    extern "C" COMPRESSION_API bool CreateTarGzArchive(
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config)
    {
        if (!Internal::g_initialized)
        {
            Internal::SetLastError("Library not initialized. Call Initialize() first.");
            return false;
        }

        try
        {
            std::string srcPath = Internal::NormalizePath(sourcePath);
            std::string dstPath = Internal::NormalizePath(destinationPath);
            std::string fullArchivePath = Internal::JoinPath(dstPath, archiveName);

            // 确保目标目录存在
            if (!Internal::CreateDirectoryRecursive(dstPath))
            {
                Internal::SetLastError("Failed to create destination directory: " + dstPath);
                return false;
            }

            // 构建文件列表
            std::vector<std::string> files;
            for (int i = 0; i < fileCount; ++i)
            {
                std::string fullPath = Internal::JoinPath(srcPath, fileList[i]);
                if (!Internal::PathExists(fullPath))
                {
                    Internal::SetLastError("File not found: " + fullPath);
                    return false;
                }
                files.push_back(fileList[i]); // 使用相对路径
            }

            return Internal::CreateTarGzWithCommand(srcPath, files, fullArchivePath);
        }
        catch (const std::exception& e)
        {
            Internal::SetLastError("Exception in CreateTarGzArchive: " + std::string(e.what()));
            return false;
        }
    }

    extern "C" COMPRESSION_API bool CreateArchive(
        ArchiveFormat format,
        const char* sourcePath,
        const char* const* fileList,
        int fileCount,
        const char* destinationPath,
        const char* archiveName,
        CompressionConfig * config)
    {
        switch (format)
        {
        case ArchiveFormat::ZIP:
            return CreateZipArchive(sourcePath, fileList, fileCount, destinationPath, archiveName, config);
        case ArchiveFormat::TAR_GZ:
            return CreateTarGzArchive(sourcePath, fileList, fileCount, destinationPath, archiveName, config);
        default:
            Internal::SetLastError("Unsupported archive format");
            return false;
        }
    }

    extern "C" COMPRESSION_API const char* GetCompressionError()
    {
        return Internal::GetLastError();
    }

    extern "C" COMPRESSION_API const char* GetCompressionVersion()
    {
        return "CompressionLib 1.0.0 (Windows Native - No External Dependencies)";
    }
}