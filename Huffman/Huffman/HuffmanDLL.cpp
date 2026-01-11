#include "pch.h"  // 这个必须放在第一行
#include "HuffmanDLL.h"

// 使用std命名空间，避免重复写std::
using namespace std;

class Huffman {
private:
    struct Node {
        uint8_t value;
        int weight;
        shared_ptr<Node> lchild;
        shared_ptr<Node> rchild;

        Node(uint8_t val, int w, shared_ptr<Node> l, shared_ptr<Node> r)
            : value(val), weight(w), lchild(l), rchild(r) {}
    };

    struct NodeCompare {
        bool operator()(const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
            return a->weight > b->weight;
        }
    };

public:
    // 计算字节频率
    static map<uint8_t, int> bytesFrequency(const vector<uint8_t>& data) {
        map<uint8_t, int> freq;
        for (uint8_t byte : data) {
            freq[byte]++;
        }
        return freq;
    }

    // 构建Huffman树并生成编码
    static map<uint8_t, string> buildHuffmanCodes(const map<uint8_t, int>& freq) {
        if (freq.empty()) return {};
        if (freq.size() == 1) {
            map<uint8_t, string> result;
            result[freq.begin()->first] = "0";
            return result;
        }

        priority_queue<shared_ptr<Node>, vector<shared_ptr<Node>>, NodeCompare> pq;

        // 创建叶子节点
        for (const auto& pair : freq) {
            pq.push(make_shared<Node>(pair.first, pair.second, nullptr, nullptr));
        }

        // 构建Huffman树
        while (pq.size() > 1) {
            auto node1 = pq.top(); pq.pop();
            auto node2 = pq.top(); pq.pop();

            auto parent = make_shared<Node>(0, node1->weight + node2->weight, node1, node2);
            pq.push(parent);
        }

        // 生成编码
        map<uint8_t, string> codes;
        generateCodes(pq.top(), "", codes);
        return codes;
    }

    // 转换为规范Huffman编码
    static map<uint8_t, string> toCanonical(const map<uint8_t, string>& codes) {
        vector<pair<uint8_t, int>> codeList;
        for (const auto& pair : codes) {
            codeList.push_back(make_pair(pair.first, static_cast<int>(pair.second.length())));
        }

        // 按编码长度和值排序
        sort(codeList.begin(), codeList.end(),
            [](const pair<uint8_t, int>& a, const pair<uint8_t, int>& b) {
                if (a.second == b.second) {
                    return a.first < b.first;
                }
                return a.second < b.second;
            });

        vector<uint8_t> values;
        vector<int> lengths;
        for (const auto& item : codeList) {
            values.push_back(item.first);
            lengths.push_back(item.second);
        }

        return rebuildCanonical(values, lengths);
    }

    // 编码数据
    static pair<vector<uint8_t>, int> encodeData(const vector<uint8_t>& data,
        const map<uint8_t, string>& codes) {
        string bitBuffer;
        vector<uint8_t> result;

        for (uint8_t byte : data) {
            auto it = codes.find(byte);
            if (it == codes.end()) {
                // 如果找不到编码，跳过这个字节（理论上不应该发生）
                continue;
            }
            bitBuffer += it->second;

            while (bitBuffer.length() >= 8) {
                bitset<8> bits(bitBuffer.substr(0, 8));
                result.push_back(static_cast<uint8_t>(bits.to_ulong()));
                bitBuffer = bitBuffer.substr(8);
            }
        }

        int padding = 0;
        if (!bitBuffer.empty()) {
            padding = 8 - static_cast<int>(bitBuffer.length());
            bitBuffer.append(padding, '0');
            bitset<8> bits(bitBuffer);
            result.push_back(static_cast<uint8_t>(bits.to_ulong()));
        }

        return make_pair(result, padding);
    }

    // 解码数据
    static vector<uint8_t> decodeData(const vector<uint8_t>& encodedData,
        const map<uint8_t, string>& codes,
        int padding) {
        if (codes.empty()) return {};
        if (codes.size() == 1) {
            vector<uint8_t> result(encodedData.size(), codes.begin()->first);
            return result;
        }

        // 重建Huffman树用于解码
        auto root = buildDecodeTree(codes);

        string bitStream;
        for (uint8_t byte : encodedData) {
            bitStream += bitset<8>(byte).to_string();
        }

        // 移除填充位
        if (padding > 0) {
            bitStream = bitStream.substr(0, bitStream.length() - padding);
        }

        vector<uint8_t> result;
        auto current = root;

        for (char bit : bitStream) {
            if (bit == '0') {
                current = current->lchild;
            }
            else {
                current = current->rchild;
            }

            if (current->lchild == nullptr && current->rchild == nullptr) {
                result.push_back(current->value);
                current = root;
            }
        }

        return result;
    }

    // 压缩
    static vector<uint8_t> compress(const vector<uint8_t>& data) {
        auto freq = bytesFrequency(data);
        auto codes = buildHuffmanCodes(freq);
        auto canonicalCodes = toCanonical(codes);

        // 构建编码表头信息
        auto header = buildHeader(canonicalCodes);
        auto encoded = encodeData(data, canonicalCodes);

        // 组合数据：填充位 + 头部 + 编码数据
        vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(encoded.second)); // 填充位数
        result.insert(result.end(), header.begin(), header.end());
        result.insert(result.end(), encoded.first.begin(), encoded.first.end());

        return result;
    }

    // 解压
    static vector<uint8_t> decompress(const vector<uint8_t>& compressedData) {
        if (compressedData.size() < 2) return {};

        int padding = compressedData[0];
        auto headerInfo = parseHeader(compressedData, 1);
        auto codes = headerInfo.first;
        size_t dataStart = headerInfo.second;

        if (dataStart >= compressedData.size()) {
            return {};
        }

        vector<uint8_t> encodedData(compressedData.begin() + dataStart, compressedData.end());
        return decodeData(encodedData, codes, padding);
    }

private:
    static void generateCodes(const shared_ptr<Node>& node, const string& code,
        map<uint8_t, string>& codes) {
        if (!node) return;

        if (!node->lchild && !node->rchild) {
            codes[node->value] = code;
        }
        else {
            generateCodes(node->lchild, code + "0", codes);
            generateCodes(node->rchild, code + "1", codes);
        }
    }

    static map<uint8_t, string> rebuildCanonical(const vector<uint8_t>& values,
        const vector<int>& lengths) {
        map<uint8_t, string> result;
        uint32_t currentCode = 0;

        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                int shiftBits = lengths[i] - lengths[i - 1];
                currentCode = (currentCode + 1) << shiftBits;
            }

            string code = bitset<32>(currentCode).to_string();
            code = code.substr(32 - lengths[i]);
            result[values[i]] = code;
        }

        return result;
    }

    static shared_ptr<Node> buildDecodeTree(const map<uint8_t, string>& codes) {
        auto root = make_shared<Node>(0, 0, nullptr, nullptr);

        for (const auto& pair : codes) {
            auto current = root;
            for (char bit : pair.second) {
                if (bit == '0') {
                    if (!current->lchild) {
                        current->lchild = make_shared<Node>(0, 0, nullptr, nullptr);
                    }
                    current = current->lchild;
                }
                else {
                    if (!current->rchild) {
                        current->rchild = make_shared<Node>(0, 0, nullptr, nullptr);
                    }
                    current = current->rchild;
                }
            }
            current->value = pair.first;
        }

        return root;
    }

    static vector<uint8_t> buildHeader(const map<uint8_t, string>& codes) {
        // 简化版头部：最大编码长度 + 各长度数量 + 字符列表
        int maxLen = 0;
        map<int, int> lenCount;

        for (const auto& pair : codes) {
            int len = static_cast<int>(pair.second.length());
            if (len > maxLen) {
                maxLen = len;  // 修复std::max问题，手动比较
            }
            lenCount[len]++;
        }

        vector<uint8_t> header;
        header.push_back(static_cast<uint8_t>(maxLen));

        // 各长度数量
        for (int i = 1; i <= maxLen; ++i) {
            header.push_back(static_cast<uint8_t>(lenCount[i]));
        }

        // 字符列表
        vector<uint8_t> sortedChars;
        for (const auto& pair : codes) {
            sortedChars.push_back(pair.first);
        }
        sort(sortedChars.begin(), sortedChars.end());

        header.insert(header.end(), sortedChars.begin(), sortedChars.end());

        return header;
    }

    static pair<map<uint8_t, string>, size_t> parseHeader(const vector<uint8_t>& data, size_t start) {
        if (start >= data.size()) return make_pair(map<uint8_t, string>(), start);

        int maxLen = data[start];
        vector<int> lengths;
        vector<uint8_t> chars;

        size_t pos = start + 1;

        // 读取各长度数量
        for (int i = 1; i <= maxLen; ++i) {
            if (pos >= data.size()) return make_pair(map<uint8_t, string>(), pos);
            int count = data[pos++];
            for (int j = 0; j < count; ++j) {
                lengths.push_back(i);
            }
        }

        // 读取字符
        for (size_t i = 0; i < lengths.size(); ++i) {
            if (pos >= data.size()) return make_pair(map<uint8_t, string>(), pos);
            chars.push_back(data[pos++]);
        }

        return make_pair(rebuildCanonical(chars, lengths), pos);
    }
};

// DLL 导出函数实现
HUFFMAN_API bool Huffman_CompressData(const unsigned char* inputData,
    unsigned int inputSize,
    unsigned char** outputData,
    unsigned int* outputSize) {
    try {
        vector<uint8_t> input(inputData, inputData + inputSize);
        vector<uint8_t> compressed = Huffman::compress(input);

        *outputData = new unsigned char[compressed.size()];
        copy(compressed.begin(), compressed.end(), *outputData);
        *outputSize = static_cast<unsigned int>(compressed.size());

        return true;
    }
    catch (const exception& e) {
        cerr << "Compression error: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Unknown compression error" << endl;
        return false;
    }
}

HUFFMAN_API bool Huffman_DecompressData(const unsigned char* inputData,
    unsigned int inputSize,
    unsigned char** outputData,
    unsigned int* outputSize) {
    try {
        vector<uint8_t> input(inputData, inputData + inputSize);
        vector<uint8_t> decompressed = Huffman::decompress(input);

        *outputData = new unsigned char[decompressed.size()];
        copy(decompressed.begin(), decompressed.end(), *outputData);
        *outputSize = static_cast<unsigned int>(decompressed.size());

        return true;
    }
    catch (const exception& e) {
        cerr << "Decompression error: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Unknown decompression error" << endl;
        return false;
    }
}

HUFFMAN_API bool Huffman_CompressFile(const char* inputPath, const char* outputPath) {
    try {
        ifstream inputFile(inputPath, ios::binary);
        if (!inputFile) {
            cerr << "Cannot open input file: " << inputPath << endl;
            return false;
        }

        inputFile.seekg(0, ios::end);
        size_t size = inputFile.tellg();
        inputFile.seekg(0, ios::beg);

        vector<uint8_t> data(size);
        inputFile.read(reinterpret_cast<char*>(data.data()), size);
        inputFile.close();

        vector<uint8_t> compressed = Huffman::compress(data);

        ofstream outputFile(outputPath, ios::binary);
        if (!outputFile) {
            cerr << "Cannot open output file: " << outputPath << endl;
            return false;
        }

        outputFile.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        outputFile.close();

        return true;
    }
    catch (const exception& e) {
        cerr << "File compression error: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Unknown file compression error" << endl;
        return false;
    }
}

HUFFMAN_API bool Huffman_DecompressFile(const char* inputPath, const char* outputPath) {
    try {
        ifstream inputFile(inputPath, ios::binary);
        if (!inputFile) {
            cerr << "Cannot open input file: " << inputPath << endl;
            return false;
        }

        inputFile.seekg(0, ios::end);
        size_t size = inputFile.tellg();
        inputFile.seekg(0, ios::beg);

        vector<uint8_t> data(size);
        inputFile.read(reinterpret_cast<char*>(data.data()), size);
        inputFile.close();

        vector<uint8_t> decompressed = Huffman::decompress(data);

        ofstream outputFile(outputPath, ios::binary);
        if (!outputFile) {
            cerr << "Cannot open output file: " << outputPath << endl;
            return false;
        }

        outputFile.write(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
        outputFile.close();

        return true;
    }
    catch (const exception& e) {
        cerr << "File decompression error: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Unknown file decompression error" << endl;
        return false;
    }
}

HUFFMAN_API void Huffman_FreeMemory(unsigned char* data) {
    delete[] data;
}