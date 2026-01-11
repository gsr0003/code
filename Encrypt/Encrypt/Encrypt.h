#pragma once

#ifndef ENCRYPT_d
#define ENCRYPT_d
#endif

#ifdef ENCRYPT_EXPORTS
#define ENCRYPT_API __declspec(dllexport)
#else
#define ENCRYPT_API __declspec(dllimport)
#endif

#include <string>

extern "C" {
	ENCRYPT_API bool encrypt3DES(const char* filePath, const char* fileName, const char* dstPath);
	ENCRYPT_API bool encryptAES(const char* filePath, const char* fileName, const char* dstPath);
	ENCRYPT_API const char* getVersion();
	ENCRYPT_API bool initialize();
	ENCRYPT_API void cleanup();
}