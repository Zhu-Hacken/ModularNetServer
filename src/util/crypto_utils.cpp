#include "crypto_utils.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

std::string CryptoUtils::hmacSha256(const std::string& data, const std::string& key) {
    unsigned char result[EVP_MAX_MD_SIZE];  // 缓冲区
    unsigned int result_len = 0;

    // 调用 OpenSSL 的 HMAC 接口

    HMAC(
        EVP_sha256(),                       // 使用 SHA256
        key.data(), key.size(),             // 密钥
        reinterpret_cast<const unsigned char*>(data.data()), data.size(),   // 数据
        result, &result_len                 // 输出与输出长度
    );

    // 转为十六进制字符串
    std::ostringstream oss;
    for (unsigned int i = 0; i < result_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    }

    return oss.str();
}

std::string CryptoUtils::base64Encode(const std::string& input) {
    int encoded_len = 4 * ((input.size() + 2) / 3);
    unsigned char* out = new unsigned char[encoded_len + 1];

    int out_len = EVP_EncodeBlock(out, 
                    reinterpret_cast<const unsigned char*>(input.data()),
                    static_cast<int>(input.size()));

    std::string result(reinterpret_cast<char*>(out), out_len);
    delete[] out;
    return result;
}