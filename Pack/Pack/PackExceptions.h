#pragma once
#include <stdexcept>
#include <string>

namespace CompressionLib
{
    class CompressionException : public std::runtime_error
    {
    public:
        explicit CompressionException(const std::string& message)
            : std::runtime_error(message) {}
    };

    class FileNotFoundException : public CompressionException
    {
    public:
        explicit FileNotFoundException(const std::string& filename)
            : CompressionException("File not found: " + filename) {}
    };

    class ArchiveCreationException : public CompressionException
    {
    public:
        explicit ArchiveCreationException(const std::string& operation)
            : CompressionException("Failed to create archive: " + operation) {}
    };
}