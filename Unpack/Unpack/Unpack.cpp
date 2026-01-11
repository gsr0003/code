#include "pch.h"
#include "Unpack.h"
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <fstream>
#include <string>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

// 在实现文件中重新定义MINIMAL_API为导出
#ifdef MINIMALARCHIVE_EXPORTS
#undef MINIMAL_API
#define MINIMAL_API __declspec(dllexport)
#endif

// 兼容性文件系统操作类
class FileSystemCompat {
public:
    static bool CreateDirectoryRecursive(const std::string& path) {
        std::string current;
        for (size_t i = 0; i < path.length(); ++i) {
            current += path[i];
            if (path[i] == '\\' || path[i] == '/' || i == path.length() - 1) {
                if (!CreateDirectoryA(current.c_str(), NULL)) {
                    DWORD err = GetLastError();
                    if (err != ERROR_ALREADY_EXISTS) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    static bool FileExists(const std::string& path) {
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
    }

    static bool DirectoryExists(const std::string& path) {
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
    }

    static std::string GetExtension(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string ext = filename.substr(dotPos);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        }
        return "";
    }

    static std::string GetFileName(const std::string& path) {
        size_t slashPos = path.find_last_of("\\/");
        if (slashPos != std::string::npos) {
            return path.substr(slashPos + 1);
        }
        return path;
    }

    static std::string GetStem(const std::string& filename) {
        std::string name = GetFileName(filename);
        size_t dotPos = name.find_last_of('.');
        if (dotPos != std::string::npos) {
            return name.substr(0, dotPos);
        }
        return name;
    }
};

class MinimalExtractorImpl : public IMinimalExtractor {
private:
    std::string base_path;
    std::string last_error;

    // 使用PowerShell解压ZIP
    bool extractWithPowerShell(const std::string& archivePath, const std::string& destPath) {
        std::string command = "powershell -command \"& {Add-Type -AssemblyName System.IO.Compression.FileSystem; ";
        command += "[System.IO.Compression.ZipFile]::ExtractToDirectory('";
        command += archivePath + "', '" + destPath + "');}\"";

        return executeCommand(command);
    }

    // 使用Windows内置的tar命令
    bool extractWithTar(const std::string& archivePath, const std::string& destPath) {
        std::string command = "tar -xf \"" + archivePath + "\" -C \"" + destPath + "\"";
        return executeCommand(command);
    }

    // 使用Expand-Archive PowerShell命令
    bool extractWithExpandArchive(const std::string& archivePath, const std::string& destPath) {
        std::string command = "powershell -command \"Expand-Archive -Path '" +
            archivePath + "' -DestinationPath '" + destPath + "' -Force\"";
        return executeCommand(command);
    }

    // 使用7-Zip（如果安装）
    bool extractWith7Zip(const std::string& archivePath, const std::string& destPath) {
        // 检查7-Zip是否安装
        const char* sevenZipPaths[] = {
            "C:\\Program Files\\7-Zip\\7z.exe",
            "C:\\Program Files (x86)\\7-Zip\\7z.exe"
        };

        std::string sevenZipPath;
        for (const char* path : sevenZipPaths) {
            if (FileExists(path)) {
                sevenZipPath = path;
                break;
            }
        }

        if (sevenZipPath.empty()) {
            return false; // 7-Zip未安装
        }

        std::string command = "\"" + sevenZipPath + "\" x \"" + archivePath + "\" -o\"" + destPath + "\" -y";
        return executeCommand(command);
    }

    // 执行系统命令
    bool executeCommand(const std::string& command) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        // 创建可修改的命令字符串
        char* cmd = new char[command.length() + 1];
        strcpy_s(cmd, command.length() + 1, command.c_str());

        BOOL success = CreateProcessA(
            NULL,           // 不使用模块名
            cmd,            // 命令行
            NULL,           // 进程句柄不可继承
            NULL,           // 线程句柄不可继承
            FALSE,          // 不继承句柄
            CREATE_NO_WINDOW, // 创建标志
            NULL,           // 使用父进程环境块
            NULL,           // 使用父进程目录
            &si,            // 指向STARTUPINFO的指针
            &pi             // 指向PROCESS_INFORMATION的指针
        );

        delete[] cmd;

        if (!success) {
            last_error = "Cannot create process for command execution";
            return false;
        }

        // 等待命令完成（30秒超时）
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 30000);
        if (waitResult == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, 1);
            last_error = "Command execution timeout";
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return false;
        }

        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0) {
            last_error = "Command failed with exit code: " + std::to_string(exitCode);
            return false;
        }

        return true;
    }

    // 检查文件是否存在
    bool FileExists(const std::string& path) {
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
    }

    // 获取文件格式
    std::string getFileFormat(const std::string& filename) {
        std::string ext = FileSystemCompat::GetExtension(filename);

        if (ext == ".zip") return "zip";
        if (ext == ".rar") return "rar";
        if (ext == ".7z") return "7z";
        if (ext == ".tar") return "tar";
        if (ext == ".gz") return "gz";

        // 处理复合扩展名
        std::string stem = FileSystemCompat::GetStem(filename);
        std::string stem_ext = FileSystemCompat::GetExtension(stem);

        if (stem_ext == ".tar") {
            if (ext == ".gz") return "tar.gz";
            if (ext == ".bz2") return "tar.bz2";
            if (ext == ".xz") return "tar.xz";
        }

        if (ext == ".tgz") return "tar.gz";
        if (ext == ".tbz2") return "tar.bz2";
        if (ext == ".txz") return "tar.xz";

        return ext;
    }

    // 规范化路径
    void normalizePath(std::string& path) {
        if (path.empty()) return;

        // 替换斜杠
        for (char& c : path) {
            if (c == '/') c = '\\';
        }

        // 确保以反斜杠结尾
        if (path.back() != '\\') {
            path += '\\';
        }
    }

public:
    MinimalExtractorImpl(const std::string& path) : base_path(path) {
        normalizePath(base_path);
        // 确保目录存在
        FileSystemCompat::CreateDirectoryRecursive(base_path);
    }

    virtual ~MinimalExtractorImpl() = default;

    void setExtractionPath(const std::string& path) override {
        base_path = path;
        normalizePath(base_path);
        FileSystemCompat::CreateDirectoryRecursive(base_path);
    }

    bool unpackFile(const std::string& filename) override {
        last_error.clear();

        std::string full_path = base_path + filename;
        std::string format = getFileFormat(filename);

        // 检查文件是否存在
        if (!FileSystemCompat::FileExists(full_path)) {
            last_error = "File does not exist: " + full_path;
            return false;
        }

        // 确保目标目录存在
        if (!FileSystemCompat::CreateDirectoryRecursive(base_path)) {
            last_error = "Cannot create destination directory";
            return false;
        }

        bool success = false;

        if (format == "zip") {
            // 方法1: 使用Expand-Archive（Windows 10+）
            success = extractWithExpandArchive(full_path, base_path);

            // 方法2: 使用.NET ZipFile类（备用）
            if (!success) {
                success = extractWithPowerShell(full_path, base_path);
            }
        }
        else if (format == "tar" || format == "tar.gz" || format == "tar.bz2" || format == "tgz") {
            // 使用tar命令（Windows 10+）
            success = extractWithTar(full_path, base_path);
        }
        else if (format == "rar" || format == "7z") {
            // 方法1: 尝试使用7-Zip
            success = extractWith7Zip(full_path, base_path);

            if (!success) {
                last_error = format + " extraction requires 7-Zip to be installed. ";
                last_error += "Please install 7-Zip and ensure it's in the default location.";
                return false;
            }
        }
        else {
            last_error = "Unsupported archive format: " + format;
            return false;
        }

        if (!success && last_error.empty()) {
            last_error = "Extraction failed for unknown reason";
        }

        return success;
    }

    std::string getLastError() const override {
        return last_error;
    }

    bool isFormatSupported(const std::string& filename) override {
        std::string format = getFileFormat(filename);
        return (format == "zip" || format == "tar" || format == "tar.gz" ||
            format == "tgz" || format == "rar" || format == "7z");
    }
};

// C风格导出函数实现
extern "C" {
    MINIMAL_API void* CreateMinimalExtractor(const char* basePath) {
        try {
            return new MinimalExtractorImpl(basePath ? basePath : "");
        }
        catch (...) {
            return nullptr;
        }
    }

    MINIMAL_API void DestroyMinimalExtractor(void* extractor) {
        if (extractor) {
            delete static_cast<MinimalExtractorImpl*>(extractor);
        }
    }

    MINIMAL_API bool UnpackArchive(void* extractor, const char* filename) {
        if (!extractor || !filename) return false;
        return static_cast<MinimalExtractorImpl*>(extractor)->unpackFile(filename);
    }

    MINIMAL_API void SetExtractionPath(void* extractor, const char* path) {
        if (extractor && path) {
            static_cast<MinimalExtractorImpl*>(extractor)->setExtractionPath(path);
        }
    }

    MINIMAL_API const char* GetLastErrorMessage(void* extractor) {
        if (!extractor) return "Invalid extractor instance";
        static std::string error;
        error = static_cast<MinimalExtractorImpl*>(extractor)->getLastError();
        return error.c_str();
    }

    MINIMAL_API bool IsFormatSupported(const char* filename) {
        if (!filename) return false;
        MinimalExtractorImpl temp_extractor("");
        return temp_extractor.isFormatSupported(filename);
    }
}

// C++接口导出函数
MINIMAL_API IMinimalExtractor* CreateMinimalExtractorInterface(const std::string& basePath) {
    return new MinimalExtractorImpl(basePath);
}

MINIMAL_API void DestroyMinimalExtractorInterface(IMinimalExtractor* extractor) {
    delete extractor;
}