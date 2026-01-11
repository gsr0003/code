#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <algorithm>

// 第三方库头文件
#include <zip.h>
#include <archive.h>
#include <archive_entry.h>
#include <cstring>

namespace fs = std::filesystem;

class ArchivePacker {
private:
    // 检查路径是否在指定根目录下
    static bool isPathSafe(const fs::path& filepath, const fs::path& root) {
        fs::path canonical_file = fs::canonical(filepath);
        fs::path canonical_root = fs::canonical(root);
        
        // 检查文件路径是否以根路径开头
        auto it = std::search(
            canonical_file.string().begin(), canonical_file.string().end(),
            canonical_root.string().begin(), canonical_root.string().end()
        );
        
        return it == canonical_file.string().begin();
    }

public:
    // ZIP打包（支持递归目录）
    static void packZipFile(const std::string& file_path, 
                           const std::vector<std::string>& file_list,
                           const std::string& dst_path,
                           const std::string& zip_name) {
        
        fs::path output_path = fs::path(dst_path) / zip_name;
        
        try {
            std::cout << "正在创建ZIP文件: " << output_path.string() << std::endl;
            
            // 创建ZIP文件
            int error = 0;
            zip_t* zip = zip_open(output_path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
            if (!zip) {
                throw std::runtime_error("无法创建ZIP文件，错误代码: " + std::to_string(error));
            }
            
            // 设置ZIP文件注释（可选）
            zip_set_archive_comment(zip, "Created by ArchivePacker", 22);
            
            // 处理每个要打包的文件/目录
            for (const auto& file : file_list) {
                fs::path source_path = fs::path(file_path) / file;
                
                if (!fs::exists(source_path)) {
                    std::cerr << "警告: 文件不存在: " << source_path.string() << std::endl;
                    continue;
                }
                
                // 安全检查
                if (!isPathSafe(source_path, file_path)) {
                    std::cerr << "警告: 不安全路径，跳过: " << source_path.string() << std::endl;
                    continue;
                }
                
                std::cout << "添加: " << source_path.string() << std::endl;
                
                if (fs::is_directory(source_path)) {
                    // 递归添加目录
                    addDirectoryToZip(zip, source_path, file_path, file);
                } else {
                    // 添加单个文件
                    addFileToZip(zip, source_path, file_path, file);
                }
            }
            
            // 关闭ZIP文件
            if (zip_close(zip) != 0) {
                zip_discard(zip);
                throw std::runtime_error("保存ZIP文件失败");
            }
            
            std::cout << "ZIP打包完成: " << output_path.string() << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "ZIP打包错误: " << e.what() << std::endl;
            throw;
        }
    }
    
private:
    // 递归添加目录到ZIP
    static void addDirectoryToZip(zip_t* zip, const fs::path& dir_path,
                                 const fs::path& base_path, const std::string& relative_path) {
        
        // 在ZIP中创建目录条目
        std::string zip_dir_path = relative_path + "/";
        zip_source_t* source = zip_source_buffer(zip, nullptr, 0, 0);
        if (!source) {
            throw std::runtime_error("无法创建ZIP目录条目: " + zip_dir_path);
        }
        
        zip_int64_t index = zip_file_add(zip, zip_dir_path.c_str(), source, ZIP_FL_OVERWRITE);
        if (index < 0) {
            zip_source_free(source);
            throw std::runtime_error("无法添加目录到ZIP: " + zip_dir_path);
        }
        
        // 设置目录属性
        zip_set_file_compression(zip, index, ZIP_CM_DEFAULT, 0);
        
        // 递归处理目录内容
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (fs::is_directory(entry)) {
                // 创建子目录
                fs::path rel_path = fs::relative(entry.path(), base_path);
                std::string zip_entry_path = rel_path.string() + "/";
                
                zip_source_t* dir_source = zip_source_buffer(zip, nullptr, 0, 0);
                if (dir_source) {
                    zip_int64_t dir_index = zip_file_add(zip, zip_entry_path.c_str(), dir_source, ZIP_FL_OVERWRITE);
                    if (dir_index >= 0) {
                        zip_set_file_compression(zip, dir_index, ZIP_CM_DEFAULT, 0);
                    } else {
                        zip_source_free(dir_source);
                    }
                }
            } else if (fs::is_regular_file(entry)) {
                // 添加文件
                fs::path rel_path = fs::relative(entry.path(), base_path);
                addFileToZip(zip, entry.path(), base_path, rel_path.string());
            }
        }
    }
    
    // 添加单个文件到ZIP
    static void addFileToZip(zip_t* zip, const fs::path& file_path,
                            const fs::path& base_path, const std::string& relative_path) {
        
        // 读取文件内容
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开文件: " + file_path.string());
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("读取文件失败: " + file_path.string());
        }
        
        // 创建ZIP源
        zip_source_t* source = zip_source_buffer(zip, buffer.data(), size, 0);
        if (!source) {
            throw std::runtime_error("无法创建ZIP源: " + relative_path);
        }
        
        // 添加到ZIP
        zip_int64_t index = zip_file_add(zip, relative_path.c_str(), source, ZIP_FL_OVERWRITE);
        if (index < 0) {
            zip_source_free(source);
            throw std::runtime_error("无法添加文件到ZIP: " + relative_path);
        }
        
        // 设置压缩方式（DEFLATE）
        zip_set_file_compression(zip, index, ZIP_CM_DEFLATE, 6);
        
        std::cout << "  -> ZIP添加: " << relative_path 
                  << " (" << size << " 字节)" << std::endl;
    }

public:
    // TAR.GZ打包
    static void packTarFile(const std::string& file_path,
                           const std::vector<std::string>& file_list,
                           const std::string& dst_path,
                           const std::string& tgz_name,
                           bool recursive = true) {
        
        fs::path output_path = fs::path(dst_path) / tgz_name;
        
        try {
            std::cout << "正在创建TAR.GZ文件: " << output_path.string() << std::endl;
            
            // 创建归档对象
            struct archive* a = archive_write_new();
            archive_write_set_format_pax_restricted(a);  // PAX格式
            archive_write_add_filter_gzip(a);            // GZIP压缩
            
            // 打开输出文件
            if (archive_write_open_filename(a, output_path.string().c_str()) != ARCHIVE_OK) {
                archive_write_free(a);
                throw std::runtime_error("无法创建TAR.GZ文件");
            }
            
            // 处理每个要打包的文件/目录
            size_t total_files = 0;
            for (const auto& file : file_list) {
                fs::path source_path = fs::path(file_path) / file;
                
                if (!fs::exists(source_path)) {
                    std::cerr << "警告: 文件不存在: " << source_path.string() << std::endl;
                    continue;
                }
                
                // 安全检查
                if (!isPathSafe(source_path, file_path)) {
                    std::cerr << "警告: 不安全路径，跳过: " << source_path.string() << std::endl;
                    continue;
                }
                
                std::cout << "添加: " << source_path.string() << std::endl;
                
                if (recursive) {
                    // 递归添加目录
                    total_files += addDirectoryToTar(a, source_path, file_path, file);
                } else {
                    // 仅添加第一级
                    total_files += addFileOrDirToTar(a, source_path, file_path, file);
                }
            }
            
            // 关闭归档
            archive_write_close(a);
            archive_write_free(a);
            
            std::cout << "TAR.GZ打包完成: " << output_path.string() 
                      << " (共 " << total_files << " 个文件)" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "TAR打包错误: " << e.what() << std::endl;
            throw;
        }
    }

private:
    // 递归添加目录到TAR
    static size_t addDirectoryToTar(struct archive* a, const fs::path& dir_path,
                                   const fs::path& base_path, const std::string& relative_path) {
        
        size_t file_count = 0;
        
        // 添加目录条目本身
        if (addFileOrDirToTar(a, dir_path, base_path, relative_path)) {
            file_count++;
        }
        
        // 递归处理目录内容
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (fs::exists(entry) && !fs::is_symlink(entry)) {
                fs::path rel_path = fs::relative(entry.path(), base_path);
                if (addFileOrDirToTar(a, entry.path(), base_path, rel_path.string())) {
                    file_count++;
                }
            }
        }
        
        return file_count;
    }
    
    // 添加文件或目录到TAR
    static bool addFileOrDirToTar(struct archive* a, const fs::path& entry_path,
                                 const fs::path& base_path, const std::string& relative_path) {
        
        // 创建归档条目
        struct archive_entry* entry = archive_entry_new();
        if (!entry) {
            return false;
        }
        
        try {
            // 设置条目名称（使用基本文件名或完整路径）
            // 根据原始Python代码的行为，这里使用基本文件名
            std::string entry_name = fs::path(relative_path).filename().string();
            archive_entry_set_pathname(entry, entry_name.c_str());
            
            // 获取文件状态
            struct stat st;
            if (stat(entry_path.string().c_str(), &st) != 0) {
                archive_entry_free(entry);
                return false;
            }
            
            // 设置条目属性
            archive_entry_copy_stat(entry, &st);
            
            // 处理符号链接
            if (S_ISLNK(st.st_mode)) {
                char link_target[1024];
                ssize_t len = readlink(entry_path.string().c_str(), link_target, sizeof(link_target) - 1);
                if (len > 0) {
                    link_target[len] = '\0';
                    archive_entry_set_symlink(entry, link_target);
                }
            }
            
            // 写入条目头部
            if (archive_write_header(a, entry) != ARCHIVE_OK) {
                archive_entry_free(entry);
                return false;
            }
            
            // 如果是普通文件，写入内容
            if (S_ISREG(st.st_mode)) {
                std::ifstream file(entry_path, std::ios::binary);
                if (file.is_open()) {
                    char buffer[8192];
                    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                        archive_write_data(a, buffer, file.gcount());
                    }
                }
            }
            
            std::cout << "  -> TAR添加: " << entry_name 
                      << " (" << st.st_size << " 字节)" << std::endl;
            
            archive_entry_free(entry);
            return true;
            
        } catch (...) {
            archive_entry_free(entry);
            throw;
        }
    }

public:
    // 创建测试文件
    static void createTestFiles(const std::string& base_path) {
        fs::create_directories(base_path);
        
        // 创建测试文件
        std::vector<std::string> test_files = {
            "file1.txt",
            "file2.txt",
            "dir1/file3.txt",
            "dir1/subdir/file4.txt",
            "dir2/file5.txt"
        };
        
        for (const auto& file : test_files) {
            fs::path file_path = fs::path(base_path) / file;
            fs::create_directories(file_path.parent_path());
            
            std::ofstream out(file_path);
            if (out.is_open()) {
                out << "This is test file: " << file << "\n";
                out << "Created for archive testing.\n";
                out.close();
            }
        }
        
        std::cout << "测试文件已创建在: " << base_path << std::endl;
    }
    
    // 列出归档文件内容
    static void listArchiveContents(const std::string& archive_path) {
        struct archive* a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        
        if (archive_read_open_filename(a, archive_path.c_str(), 10240) != ARCHIVE_OK) {
            archive_read_free(a);
            throw std::runtime_error("无法打开归档文件");
        }
        
        std::cout << "归档文件内容: " << archive_path << std::endl;
        std::cout << "----------------------------------------\n";
        
        struct archive_entry* entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            const char* filename = archive_entry_pathname(entry);
            int64_t size = archive_entry_size(entry);
            mode_t mode = archive_entry_mode(entry);
            
            std::string type = "文件";
            if (S_ISDIR(mode)) type = "目录";
            if (S_ISLNK(mode)) type = "链接";
            
            std::cout << type << ": " << filename;
            if (!S_ISDIR(mode)) {
                std::cout << " (" << size << " 字节)";
            }
            std::cout << std::endl;
            
            archive_read_data_skip(a);
        }
        
        archive_read_close(a);
        archive_read_free(a);
    }
};

// 使用示例
int main() {
    std::string base_path = "./test_files";
    std::string dst_path = "./";
    
    try {
        // 创建测试文件
        ArchivePacker::createTestFiles(base_path);
        
        // 要打包的文件列表
        std::vector<std::string> file_list = {
            "file1.txt",
            "file2.txt",
            "dir1",
            "dir2"
        };
        
        std::cout << "\n=== ZIP打包测试 ===\n";
        ArchivePacker::packZipFile(base_path, file_list, dst_path, "test_archive.zip");
        
        std::cout << "\n=== TAR.GZ打包测试（递归）===\n";
        ArchivePacker::packTarFile(base_path, file_list, dst_path, "test_archive.tar.gz", true);
        
        std::cout << "\n=== TAR.GZ打包测试（非递归）===\n";
        ArchivePacker::packTarFile(base_path, file_list, dst_path, "test_archive_flat.tar.gz", false);
        
        std::cout << "\n=== 查看归档内容 ===\n";
        ArchivePacker::listArchiveContents("test_archive.zip");
        
    } catch (const std::exception& e) {
        std::cerr << "程序出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}