#pragma once

#ifndef DECRYPT_d
#define DECRYPT_d
#endif

#ifdef DECRYPT_EXPORTS
#define DECRYPT_API __declspec(dllexport)
#else
#define DECRYPT_API __declspec(dllimport)
#endif

#include <string>

extern "C" {
	DECRYPT_API bool decrypt3DES(const char* filePath, const char* fileName, const char* dstPath);
	DECRYPT_API bool decryptAES(const char* filePath, const char* fileName, const char* dstPath);
	DECRYPT_API const char* getVersion();
	DECRYPT_API bool initialize();
	DECRYPT_API void cleanup();
}