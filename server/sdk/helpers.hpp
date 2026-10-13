#pragma once
#include <string>
#include <cstddef>
#include <cstdint>

namespace sdk {
    template <typename T, size_t N>
    inline size_t array_count(const T (&)[N]) {
        return N;
    }

    inline void string_to_wchar(const std::string& s, wchar_t* buffer, size_t length) {
        if (length == 0) return;
        size_t n = s.size() < (length - 1) ? s.size() : (length - 1);
        for (size_t i = 0; i < n; i++) {
            buffer[i] = static_cast<wchar_t>(static_cast<unsigned char>(s[i]));
        }
        buffer[n] = 0;
    }

    inline void string_to_uchar(const std::string& s, unsigned char* buffer, size_t length) {
        if (length == 0) return;
        size_t n = s.size() < (length - 1) ? s.size() : (length - 1);
        for (size_t i = 0; i < n; i++) {
            buffer[i] = static_cast<unsigned char>(s[i]);
        }
        buffer[n] = 0;
    }

    inline void* number_to_pointer(uint64_t number) {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(number));
    }

    inline uint64_t pointer_to_number(const void* pointer) {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pointer));
    }

    inline std::string wchar_to_string(const wchar_t* buffer, size_t length) {
        std::string s;
        if (buffer == nullptr) return s;
        for (size_t i = 0; i < length && buffer[i] != 0; i++) {
            s += static_cast<char>(static_cast<unsigned char>(buffer[i]));
        }
        return s;
    }

    inline std::string uchar_to_string(const unsigned char* buffer, size_t length) {
        std::string s;
        if (buffer == nullptr) return s;
        for (size_t i = 0; i < length && buffer[i] != 0; i++) {
            s += static_cast<char>(buffer[i]);
        }
        return s;
    }

    inline std::string bytes_to_string(const unsigned char* buffer, size_t length) {
        if (buffer == nullptr || length == 0) return std::string();
        return std::string(reinterpret_cast<const char*>(buffer), length);
    }
} // namespace sdk
