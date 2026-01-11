#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <chrono>

// 第三方库头文件
#include <zip.h>
#include <archive.h>
#include <archive_entry.h>
#include <cstring>

namespace fs = std::filesystem;

class ArchiveExtractor {
private:
    // 解压进度回调函数类型
    using ProgressCallback = void(*)(const std::string&, size_t, size_t, void*);
    
    // 默认进度回调函数
    static void defaultProgressCallback(const std::string& filename, 
                                       size_t current, size_t total, void* userdata) {
        if (total > 0) {
            int percent = static_cast<int>((current * 100) / total);
            std::cout << "\r解压中: " << std::setw(50) << std::left 
                      << (filename.length() > 50 ? filename.substr(0, 47) + "..." : filename)
                      << " " << percent << "% (" << current << "/" << total << " 字节)";
            std::cout.flush();
        }
    }

public:
    // ZIP解包
    static void unpackZipFile(const std::string& path, const std::string& file,
                             const std::string& output_dir = "",
                             ProgressCallback progress_cb = nullptr,
                             void* userdata = nullptr) {
        
        fs::path archive_path = fs::path(path) / file;
        fs::path extract_path = output_dir.empty() ? path : output_dir;
        
        try {
            // 检查文件是否存在
            if (!fs::exists(archive_path)) {
                throw std::runtime_error("归档文件不存在: " + archive_path.string());
            }
            
            // 创建解压目录
            fs::create_directories(extract_path);
            
            std::cout << "正在解压ZIP文件: " << archive_path.string() << std::endl;
            std::cout << "解压到: " << extract_path.string() << std::endl;
            
            // 打开ZIP文件
            int error = 0;
            zip_t* zip = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error);
            if (!zip) {
                throw std::runtime_error("无法打开ZIP文件，错误代码: " + std::to_string(error));
            }
            
            // 获取文件数量
            zip_int64_t num_entries = zip_get_num_entries(zip, ZIP_FL_UNCHANGED);
            std::cout << "包含 " << num_entries << " 个文件" << std::endl;
            
            // 遍历并解压所有文件
            zip_int64_t total_extracted = 0;
            zip_int64_t total_skipped = 0;
            
            for (zip_int64_t i = 0; i < num_entries; i++) {
                const char* name = zip_get_name(zip, i, ZIP_FL_ENC_RAW);
                if (!name) {
                    std::cerr << "警告: 无法获取第 " << i << " 个条目的名称" << std::endl;
                    continue;
                }
                
                // 安全检查：防止路径遍历攻击
                if (!isSafePath(name)) {
                    std::cerr << "警告: 跳过不安全路径: " << name << std::endl;
                    total_skipped++;
                    continue;
                }
                
                fs::path full_path = extract_path / name;
                
                // 获取文件信息
                struct zip_stat st;
                zip_stat_init(&st);
                if (zip_stat_index(zip, i, 0, &st) != 0) {
                    std::cerr << "警告: 无法获取文件信息: " << name << std::endl;
                    continue;
                }
                
                // 检查是否是目录
                if (st.name[strlen(st.name) - 1] == '/') {
                    // 创建目录
                    fs::create_directories(full_path);
                    std::cout << "创建目录: " << name << std::endl;
                } else {
                    // 解压文件
                    if (extractZipEntry(zip, i, st, full_path, progress_cb, userdata)) {
                        total_extracted++;
                    } else {
                        std::cerr << "错误: 解压失败: " << name << std::endl;
                    }
                }
            }
            
            // 关闭ZIP文件
            zip_close(zip);
            
            std::cout << "\n解压完成!" << std::endl;
            std::cout << "成功解压: " << total_extracted << " 个文件" << std::endl;
            if (total_skipped > 0) {
                std::cout << "跳过: " << total_skipped << " 个不安全文件" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "\nZIP解压错误: " << e.what() << std::endl;
            throw;
        }
    }

private:
    // 提取ZIP条目
    static bool extractZipEntry(zip_t* zip, zip_int64_t index, 
                               const struct zip_stat& st,
                               const fs::path& output_path,
                               ProgressCallback progress_cb,
                               void* userdata) {
        
        // 确保父目录存在
        fs::create_directories(output_path.parent_path());
        
        // 打开ZIP条目
        zip_file_t* zf = zip_fopen_index(zip, index, 0);
        if (!zf) {
            return false;
        }
        
        try {
            std::cout << "解压文件: " << st.name << " (" << st.size << " 字节)" << std::endl;
            
            // 创建输出文件
            std::ofstream out_file(output_path, std::ios::binary);
            if (!out_file.is_open()) {
                zip_fclose(zf);
                return false;
            }
            
            // 读取并写入数据
            char buffer[8192];
            zip_int64_t total_read = 0;
            
            while (total_read < st.size) {
                zip_int64_t bytes_read = zip_fread(zf, buffer, sizeof(buffer));
                if (bytes_read < 0) {
                    zip_fclose(zf);
                    return false;
                }
                
                out_file.write(buffer, bytes_read);
                total_read += bytes_read;
                
                // 调用进度回调
                if (progress_cb) {
                    progress_cb(st.name, total_read, st.size, userdata);
                }
            }
            
            zip_fclose(zf);
            
            // 设置文件权限（如果可用）
            if (st.valid & ZIP_STAT_MTIME) {
                // 设置修改时间
                // 注意：fs::last_write_time需要C++20完全支持
            }
            
            return true;
            
        } catch (...) {
            zip_fclose(zf);
            throw;
        }
    }
    
    // 检查路径是否安全
    static bool isSafePath(const std::string& path) {
        // 检查路径遍历攻击（如包含..或绝对路径）
        if (path.find("..") != std::string::npos) {
            return false;
        }
        
        // 检查绝对路径
        if (path.length() > 0 && (path[0] == '/' || path[0] == '\\')) {
            return false;
        }
        
        // 检查Windows驱动器路径
        if (path.length() > 1 && path[1] == ':') {
            return false;
        }
        
        return true;
    }

public:
    // TAR.GZ解包
    static void unpackTarFile(const std::string& path, const std::string& file,
                             const std::string& output_dir = "",
                             bool strip_components = 0,
                             ProgressCallback progress_cb = nullptr,
                             void* userdata = nullptr) {
        
        fs::path archive_path = fs::path(path) / file;
        fs::path extract_path = output_dir.empty() ? path : output_dir;
        
        try {
            // 检查文件是否存在
            if (!fs::exists(archive_path)) {
                throw std::runtime_error("归档文件不存在: " + archive_path.string());
            }
            
            // 创建解压目录
            fs::create_directories(extract_path);
            
            std::cout << "正在解压TAR.GZ文件: " << archive_path.string() << std::endl;
            std::cout << "解压到: " << extract_path.string() << std::endl;
            
            // 创建读取归档对象
            struct archive* a = archive_read_new();
            archive_read_support_format_all(a);
            archive_read_support_filter_all(a);
            
            // 打开归档文件
            if (archive_read_open_filename(a, archive_path.string().c_str(), 10240) != ARCHIVE_OK) {
                archive_read_free(a);
                throw std::runtime_error("无法打开TAR.GZ文件");
            }
            
            // 创建写入归档对象（用于提取）
            struct archive* ext = archive_write_disk_new();
            archive_write_disk_set_options(ext, 
                ARCHIVE_EXTRACT_TIME |
                ARCHIVE_EXTRACT_PERM |
                ARCHIVE_EXTRACT_ACL |
                ARCHIVE_EXTRACT_FFLAGS |
                ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                ARCHIVE_EXTRACT_SECURE_NODOTDOT);
            
            // 设置解压目录
            archive_write_disk_set_standard_lookup(ext);
            
            // 遍历并解压所有文件
            size_t total_extracted = 0;
            size_t total_skipped = 0;
            
            struct archive_entry* entry;
            while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                const char* entry_path = archive_entry_pathname(entry);
                
                // 安全检查
                if (!isSafePath(entry_path)) {
                    std::cerr << "警告: 跳过不安全路径: " << entry_path << std::endl;
                    archive_read_data_skip(a);
                    total_skipped++;
                    continue;
                }
                
                // 去除指定层级的目录（strip_components）
                std::string adjusted_path = entry_path;
                if (strip_components > 0) {
                    adjusted_path = stripPathComponents(entry_path, strip_components);
                    if (adjusted_path.empty()) {
                        archive_read_data_skip(a);
                        continue;
                    }
                }
                
                // 构建完整输出路径
                fs::path full_path = extract_path / adjusted_path;
                
                // 更新归档条目路径
                archive_entry_set_pathname(entry, full_path.string().c_str());
                
                // 提取文件
                if (extractArchiveEntry(a, ext, entry, progress_cb, userdata)) {
                    total_extracted++;
                } else {
                    std::cerr << "错误: 解压失败: " << entry_path << std::endl;
                }
            }
            
            // 清理资源
            archive_read_close(a);
            archive_read_free(a);
            archive_write_close(ext);
            archive_write_free(ext);
            
            std::cout << "\n解压完成!" << std::endl;
            std::cout << "成功解压: " << total_extracted << " 个文件" << std::endl;
            if (total_skipped > 0) {
                std::cout << "跳过: " << total_skipped << " 个不安全文件" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "\nTAR解压错误: " << e.what() << std::endl;
            throw;
        }
    }

private:
    // 去除路径的前几个组件
    static std::string stripPathComponents(const std::string& path, int components) {
        std::string result = path;
        
        for (int i = 0; i < components; i++) {
            size_t pos = result.find_first_of("/\\");
            if (pos == std::string::npos) {
                return "";  // 没有足够的组件可以去除
            }
            result = result.substr(pos + 1);
        }
        
        return result;
    }
    
    // 提取归档条目
    static bool extractArchiveEntry(struct archive* a, struct archive* ext,
                                   struct archive_entry* entry,
                                   ProgressCallback progress_cb,
                                   void* userdata) {
        
        // 写入条目头部
        int r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "警告: 无法写入文件头: " 
                      << archive_entry_pathname(entry) 
                      << " - " << archive_error_string(ext) << std::endl;
            return false;
        }
        
        // 如果是普通文件，写入内容
        const void* buff;
        size_t size;
        la_int64_t offset;
        
        if (archive_entry_size(entry) > 0) {
            const char* filename = archive_entry_pathname(entry);
            la_int64_t total_size = archive_entry_size(entry);
            la_int64_t bytes_written = 0;
            
            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) {
                    break;
                }
                if (r != ARCHIVE_OK) {
                    std::cerr << "警告: 读取数据块失败: " 
                              << archive_error_string(a) << std::endl;
                    return false;
                }
                
                r = archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    std::cerr << "警告: 写入数据块失败: " 
                              << archive_error_string(ext) << std::endl;
                    return false;
                }
                
                bytes_written += size;
                
                // 调用进度回调
                if (progress_cb) {
                    progress_cb(filename, bytes_written, total_size, userdata);
                }
            }
        }
        
        // 完成条目写入
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
            std::cerr << "警告: 完成条目失败: " 
                      << archive_error_string(ext) << std::endl;
            return false;
        }
        
        std::cout << "解压文件: " << archive_entry_pathname(entry) 
                  << " (" << archive_entry_size(entry) << " 字节)" << std::endl;
        
        return true;
    }

public:
    // 通用解压函数（自动检测格式）
    static void unpackFile(const std::string& path, const std::string& file,
                          const std::string& output_dir = "",
                          ProgressCallback progress_cb = nullptr,
                          void* userdata = nullptr) {
        
        fs::path archive_path = fs::path(path) / file;
        
        // 检测文件类型
        std::string ext = archive_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".zip") {
            unpackZipFile(path, file, output_dir, progress_cb, userdata);
        } else if (ext == ".tar" || ext == ".tar.gz" || ext == ".tgz" || 
                   ext == ".tar.bz2" || ext == ".tbz2" || ext == ".tar.xz" || ext == ".txz") {
            unpackTarFile(path, file, output_dir, 0, progress_cb, userdata);
        } else {
            throw std::runtime_error("不支持的文件格式: " + ext);
        }
    }
    
    // 列出归档内容
    static void listArchiveContents(const std::string& path, const std::string& file) {
        fs::path archive_path = fs::path(path) / file;
        
        // 检测文件类型
        std::string ext = archive_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".zip") {
            listZipContents(path, file);
        } else if (ext == ".tar" || ext == ".tar.gz" || ext == ".tgz" || 
                   ext == ".tar.bz2" || ext == ".tbz2" || ext == ".tar.xz" || ext == ".txz") {
            listTarContents(path, file);
        } else {
            throw std::runtime_error("不支持的文件格式: " + ext);
        }
    }

private:
    // 列出ZIP文件内容
    static void listZipContents(const std::string& path, const std::string& file) {
        fs::path archive_path = fs::path(path) / file;
        
        int error = 0;
        zip_t* zip = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &error);
        if (!zip) {
            throw std::runtime_error("无法打开ZIP文件");
        }
        
        std::cout << "ZIP文件内容: " << archive_path.string() << std::endl;
        std::cout << "=================================================================\n";
        
        zip_int64_t num_entries = zip_get_num_entries(zip, ZIP_FL_UNCHANGED);
        zip_int64_t total_size = 0;
        
        for (zip_int64_t i = 0; i < num_entries; i++) {
            const char* name = zip_get_name(zip, i, ZIP_FL_ENC_RAW);
            struct zip_stat st;
            zip_stat_init(&st);
            
            if (zip_stat_index(zip, i, 0, &st) == 0) {
                std::string type = "文件";
                if (name[strlen(name) - 1] == '/') type = "目录";
                
                std::cout << std::setw(10) << type 
                          << " " << std::setw(10) << st.size << " 字节"
                          << " " << name << std::endl;
                
                total_size += st.size;
            }
        }
        
        std::cout << "=================================================================\n";
        std::cout << "总计: " << num_entries << " 个条目, " 
                  << total_size << " 字节" << std::endl;
        
        zip_close(zip);
    }
    
    // 列出TAR文件内容
    static void listTarContents(const std::string& path, const std::string& file) {
        fs::path archive_path = fs::path(path) / file;
        
        struct archive* a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        
        if (archive_read_open_filename(a, archive_path.string().c_str(), 10240) != ARCHIVE_OK) {
            archive_read_free(a);
            throw std::runtime_error("无法打开归档文件");
        }
        
        std::cout << "归档文件内容: " << archive_path.string() << std::endl;
        std::cout << "=================================================================\n";
        
        struct archive_entry* entry;
        size_t total_entries = 0;
        la_int64_t total_size = 0;
        
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            const char* filename = archive_entry_pathname(entry);
            la_int64_t size = archive_entry_size(entry);
            mode_t mode = archive_entry_mode(entry);
            
            std::string type = "文件";
            if (S_ISDIR(mode)) type = "目录";
            else if (S_ISLNK(mode)) type = "链接";
            else if (S_ISCHR(mode)) type = "字符设备";
            else if (S_ISBLK(mode)) type = "块设备";
            else if (S_ISFIFO(mode)) type = "FIFO";
            else if (S_ISSOCK(mode)) type = "套接字";
            
            std::cout << std::setw(10) << type
                      << " " << std::setw(10) << size << " 字节"
                      << " " << filename;
            
            if (S_ISLNK(mode)) {
                const char* link = archive_entry_symlink(entry);
                std::cout << " -> " << link;
            }
            
            std::cout << std::endl;
            
            total_entries++;
            total_size += size;
            
            archive_read_data_skip(a);
        }
        
        std::cout << "=================================================================\n";
        std::cout << "总计: " << total_entries << " 个条目, " 
                  << total_size << " 字节" << std::endl;
        
        archive_read_close(a);
        archive_read_free(a);
    }
};

// 使用示例
int main() {
    try {
        std::string path = "./";
        
        // 进度回调结构
        struct ProgressData {
            size_t total_files;
            size_t current_file;
        };
        
        ProgressData progress_data{0, 0};
        
        // 自定义进度回调
        auto customProgressCallback = [](const std::string& filename, 
                                        size_t current, size_t total, void* userdata) {
            ProgressData* data = static_cast<ProgressData*>(userdata);
            if (total > 0) {
                int percent = static_cast<int>((current * 100) / total);
                std::cout << "\r正在解压 [" << data->current_file << "/" 
                          << data->total_files << "] " 
                          << (filename.length() > 30 ? filename.substr(0, 27) + "..." : filename)
                          << ": " << percent << "%";
                std::cout.flush();
            }
        };
        
        std::cout << "=== 归档解压工具 ===\n\n";
        
        // 列出文件
        std::cout << "当前目录中的归档文件:\n";
        for (const auto& entry : fs::directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".zip" || ext == ".tar.gz" || ext == ".tgz" || 
                    ext == ".tar" || ext == ".tar.bz2" || ext == ".tbz2") {
                    std::cout << "  " << entry.path().filename().string() << "\n";
                }
            }
        }
        
        std::cout << "\n=== 列出归档内容 ===\n";
        // 假设有一个test.zip文件
        if (fs::exists("test.zip")) {
            ArchiveExtractor::listArchiveContents(path, "test.zip");
        }
        
        std::cout << "\n=== 解压ZIP文件 ===\n";
        // 解压ZIP文件
        if (fs::exists("test.zip")) {
            ArchiveExtractor::unpackZipFile(path, "test.zip", "extracted_zip",
                                           customProgressCallback, &progress_data);
        }
        
        std::cout << "\n=== 解压TAR.GZ文件 ===\n";
        // 解压TAR.GZ文件
        if (fs::exists("test.tar.gz")) {
            ArchiveExtractor::unpackTarFile(path, "test.tar.gz", "extracted_tar",
                                          0, customProgressCallback, &progress_data);
        }
        
        std::cout << "\n=== 使用通用解压函数 ===\n";
        // 通用解压
        if (fs::exists("test.zip")) {
            ArchiveExtractor::unpackFile(path, "test.zip", "extracted_auto",
                                        customProgressCallback, &progress_data);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "程序出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}