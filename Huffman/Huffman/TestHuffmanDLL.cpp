#include "pch.h"
#include "HuffmanDLL.h"
#include <iostream>
#include <vector>

int main() {
    // 测试数据压缩
    std::string testData = "This is a test string for Huffman compression algorithm.";
    std::vector<unsigned char> data(testData.begin(), testData.end());

    unsigned char* compressedData = nullptr;
    unsigned int compressedSize = 0;

    if (Huffman_CompressData(data.data(), data.size(), &compressedData, &compressedSize)) {
        std::cout << "Compression successful!" << std::endl;
        std::cout << "Original size: " << data.size() << std::endl;
        std::cout << "Compressed size: " << compressedSize << std::endl;
        std::cout << "Compression ratio: " << (1.0 - (double)compressedSize / data.size()) * 100 << "%" << std::endl;

        // 测试解压
        unsigned char* decompressedData = nullptr;
        unsigned int decompressedSize = 0;

        if (Huffman_DecompressData(compressedData, compressedSize, &decompressedData, &decompressedSize)) {
            std::cout << "Decompression successful!" << std::endl;
            std::string result(reinterpret_cast<char*>(decompressedData), decompressedSize);
            std::cout << "Decompressed data: " << result << std::endl;

            // 释放内存
            Huffman_FreeMemory(decompressedData);
        }

        // 释放内存
        Huffman_FreeMemory(compressedData);
    }

    // 测试文件压缩
    if (Huffman_CompressFile("test_input.txt", "test_compressed.huff")) {
        std::cout << "File compression successful!" << std::endl;

        if (Huffman_DecompressFile("test_compressed.huff", "test_decompressed.txt")) {
            std::cout << "File decompression successful!" << std::endl;
        }
    }

    return 0;
}