#pragma once

#ifdef HUFFMANDLL_EXPORTS
#define HUFFMAN_API __declspec(dllexport)
#else
#define HUFFMAN_API __declspec(dllimport)
#endif

#include <vector>
#include <string>

extern "C" {

    // 压缩数据
    HUFFMAN_API bool Huffman_CompressData(const unsigned char* inputData,
        unsigned int inputSize,
        unsigned char** outputData,
        unsigned int* outputSize);

    // 解压数据
    HUFFMAN_API bool Huffman_DecompressData(const unsigned char* inputData,
        unsigned int inputSize,
        unsigned char** outputData,
        unsigned int* outputSize);

    // 压缩文件
    HUFFMAN_API bool Huffman_CompressFile(const char* inputPath, const char* outputPath);

    // 解压文件
    HUFFMAN_API bool Huffman_DecompressFile(const char* inputPath, const char* outputPath);

    // 释放内存
    HUFFMAN_API void Huffman_FreeMemory(unsigned char* data);

}