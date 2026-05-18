#include "JwtHelper.hpp"
#include <gtest/gtest.h>
#include <thread>

TEST(JwtTests, Base64UrlEncodingAndDecoding) {
    std::string original = "Hello, World! + / = 123";
    std::string encoded = JwtHelper::base64url_encode(original);
    
    // Encoded should not contain special base64 URL characters +, / or trailing =
    EXPECT_EQ(encoded.find('+'), std::string::npos);
    EXPECT_EQ(encoded.find('/'), std::string::npos);
    EXPECT_EQ(encoded.find('='), std::string::npos);

    std::string decoded = JwtHelper::base64url_decode(encoded);
    EXPECT_EQ(original, decoded);
}

TEST(JwtTests, HmacSha256GeneratesHash) {
    std::string key = "secret-key";
    std::string data = "message-payload";
    std::string hash = JwtHelper::hmac_sha256(key, data);
    
    EXPECT_FALSE(hash.empty());
    // HMAC SHA256 returns binary signature
    EXPECT_EQ(hash.length(), 32);
}

TEST(JwtTests, TokenGenerationAndVerification) {
    std::string secret = "my-secure-signing-secret-key-1029";
    std::string username = "player@test.com";
    int userId = 42;

    std::string token = JwtHelper::generate_jwt(userId, username, secret, 5); // 5 seconds lifespan
    EXPECT_FALSE(token.empty());

    // Triple section verification (header.payload.signature)
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    EXPECT_NE(firstDot, std::string::npos);
    EXPECT_NE(secondDot, std::string::npos);

    std::string verifiedUsername;
    int verifiedId = JwtHelper::verify_jwt(token, secret, verifiedUsername);
    EXPECT_EQ(verifiedId, userId);
    EXPECT_EQ(verifiedUsername, username);
}

TEST(JwtTests, ExpiredTokenVerificationFails) {
    std::string secret = "secret";
    std::string username = "expired@user.com";
    int userId = 10;

    // Generate expired token (-5 seconds expiration offset)
    std::string token = JwtHelper::generate_jwt(userId, username, secret, -5);
    
    std::string verifiedUsername;
    int verifiedId = JwtHelper::verify_jwt(token, secret, verifiedUsername);
    
    EXPECT_EQ(verifiedId, -1);
}

TEST(JwtTests, InvalidSecretVerificationFails) {
    std::string correctSecret = "correct";
    std::string invalidSecret = "wrong";
    std::string username = "test@user.com";
    int userId = 5;

    std::string token = JwtHelper::generate_jwt(userId, username, correctSecret, 100);
    
    std::string verifiedUsername;
    int verifiedId = JwtHelper::verify_jwt(token, invalidSecret, verifiedUsername);
    
    EXPECT_EQ(verifiedId, -1);
}

TEST(JwtTests, MalformedTokenVerificationFails) {
    std::string secret = "secret";
    std::string verifiedUsername;
    
    // Missing parts
    EXPECT_EQ(JwtHelper::verify_jwt("malformedTokenString", secret, verifiedUsername), -1);
    EXPECT_EQ(JwtHelper::verify_jwt("part1.part2", secret, verifiedUsername), -1);
    EXPECT_EQ(JwtHelper::verify_jwt("p1.p2.p3.p4", secret, verifiedUsername), -1);
}
