#pragma once
#include <string>

class CryptoUtils {
public:
    CryptoUtils() = delete;
    ~CryptoUtils();

    static std::string hmacSha256(const std::string& data, const std::string& key);
    static std::string base64Encode(const std::string& input);
    static std::string sha1(const std::string& input);
};
