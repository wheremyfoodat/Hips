#pragma once
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <utility>

#ifdef _MSC_VER
// 64 bit offsets for MSVC
#define fseeko _fseeki64
#define ftello _ftelli64
#define fileno _fileno

#pragma warning(disable : 4996)
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(WIN32) || defined(_WIN32)
#include <io.h> // For _chsize_s
#else
#include <unistd.h> // For ftruncate
#endif

class IOFile {
    FILE* handle = nullptr;

public:
    IOFile() {}
    IOFile(FILE* handle) : handle(handle) {}
    IOFile(const std::filesystem::path& path, const char* permissions = "rb") {
        open(path, permissions);
    }

    bool isOpen() {
        return handle != nullptr;
    }

    bool open(const std::filesystem::path& path, const char* permissions = "rb") {
        const auto str = path.string(); // For some reason converting paths directly with c_str() doesn't work
        return open(str.c_str(), permissions);
    }

    bool open(const char* filename, const char* permissions = "rb") {
        handle = std::fopen(filename, permissions);
        return isOpen();
    }

    void close() {
        if (isOpen()) {
            fclose(handle);
            handle = nullptr;
        }
    }

    std::pair<bool, std::size_t> read(void* data, std::size_t length, std::size_t dataSize) {
        if (!isOpen()) {
            return { false, std::numeric_limits<std::size_t>::max() };
        }

        if (length == 0) return { true, 0 };
        return { true, std::fread(data, dataSize, length, handle) };
    }

    auto readBytes(void* data, std::size_t count) {
        return read(data, count, sizeof(std::uint8_t));
    }

    std::pair<bool, std::size_t> write(const void* data, std::size_t length, std::size_t dataSize) {
        if (!isOpen()) {
            return { false, std::numeric_limits<std::size_t>::max() };
        }

        if (length == 0) return { true, 0 };
        return { true, std::fwrite(data, dataSize, length, handle) };
    }

    auto writeBytes(const void* data, std::size_t count) {
        return write(data, count, sizeof(std::uint8_t));
    }

    std::optional<std::uint64_t> size() {
        if (!isOpen()) return {};

        std::uint64_t pos = ftello(handle);
        if (fseeko(handle, 0, SEEK_END) != 0) {
            return {};
        }

        std::uint64_t size = ftello(handle);
        if ((size != pos) && (fseeko(handle, pos, SEEK_SET) != 0)) {
            return {};
        }

        return size;
    }

    bool seek(std::int64_t offset, int origin = SEEK_SET) {
        if (!isOpen() || fseeko(handle, offset, origin) != 0)
            return false;

        return true;
    }

    bool rewind() {
        return seek(0, SEEK_SET);
    }

    FILE* getHandle() {
        return handle;
    }

    // Sets the size of the file to "size" and returns whether it succeeded or not
    bool setSize(std::uint64_t size) {
        if (!isOpen()) return false;
        bool success;

#if defined(WIN32) || defined(_WIN32)
        success = _chsize_s(_fileno(handle), size) == 0;
#else
        success = ftruncate(fileno(handle), size) == 0;
#endif
        fflush(handle);
        return success;
    }
};