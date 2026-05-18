#pragma once
#include <string>
#include <chrono>
#include <cstring>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using json = nlohmann::json;

namespace JwtHelper {

inline std::string base64url_encode(const std::string& in) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, in.c_str(), in.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string out(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    std::string out_url = "";
    for (char c : out) {
        if (c == '+') out_url += '-';
        else if (c == '/') out_url += '_';
        else if (c == '=') continue; // Omit padding
        else out_url += c;
    }
    return out_url;
}

inline std::string base64url_decode(const std::string& in) {
    std::string b64 = in;
    for (char &c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (b64.length() % 4 != 0) {
        b64 += '=';
    }

    BIO *bio, *b64_f;
    int decodeLen = b64.length();
    char *buffer = (char *)malloc(decodeLen);
    std::memset(buffer, 0, decodeLen);

    b64_f = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(b64.c_str(), decodeLen);
    bio = BIO_push(b64_f, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int len = BIO_read(bio, buffer, decodeLen);
    std::string out(buffer, len);
    BIO_free_all(bio);
    free(buffer);
    return out;
}

inline std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.c_str(), key.length(), (const unsigned char*)data.c_str(), data.length(), hash, &len);
    return std::string((char*)hash, len);
}

inline std::string generate_jwt(int userId, const std::string& username, const std::string& secret, int exp_seconds = 3600) {
    json header = {{"alg", "HS256"}, {"typ", "JWT"}};
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + exp_seconds;
    json payload = {{"userId", userId}, {"username", username}, {"exp", exp}};

    std::string enc_header = base64url_encode(header.dump());
    std::string enc_payload = base64url_encode(payload.dump());
    std::string signature = base64url_encode(hmac_sha256(secret, enc_header + "." + enc_payload));

    return enc_header + "." + enc_payload + "." + signature;
}

inline int verify_jwt(const std::string& token, const std::string& secret, std::string& out_username) {
    size_t dot1 = token.find('.');
    if (dot1 == std::string::npos) return -1;
    size_t dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return -1;

    std::string header_p = token.substr(0, dot1);
    std::string payload_p = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string signature_p = token.substr(dot2 + 1);

    std::string expected_sig = base64url_encode(hmac_sha256(secret, header_p + "." + payload_p));
    if (signature_p != expected_sig) {
        return -1; // Invalid signature
    }

    try {
        std::string dec_payload = base64url_decode(payload_p);
        auto p = json::parse(dec_payload);
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now > p["exp"].get<long long>()) {
            return -1; // Expired
        }
        out_username = p["username"].get<std::string>();
        return p["userId"].get<int>();
    } catch (...) {
        return -1;
    }
}

} // namespace JwtHelper
