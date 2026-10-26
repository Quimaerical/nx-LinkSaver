#include "crypto/sha256.hpp"

#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cstring>

using namespace std;

namespace {
    inline uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
    inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
        return (e & f) ^ (~e & g);
    }
    inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
        return (a & b) ^ (a & c) ^ (b & c);
    }
    inline uint32_t sig0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    inline uint32_t sig1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    inline uint32_t theta0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    inline uint32_t theta1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    struct Sha256Context {
        uint32_t state[8];
        uint64_t count;
        uint8_t buffer[64];
    };

    void sha256Init(Sha256Context* ctx) {
        ctx->state[0] = 0x6a09e667;
        ctx->state[1] = 0xbb67ae85;
        ctx->state[2] = 0x3c6ef372;
        ctx->state[3] = 0xa54ff53a;
        ctx->state[4] = 0x510e527f;
        ctx->state[5] = 0x9b05688c;
        ctx->state[6] = 0x1f83d9ab;
        ctx->state[7] = 0x5be0cd19;
        ctx->count = 0;
    }

    void sha256Transform(uint32_t state[8], const uint8_t data[64]) {
        uint32_t W[64];
        for (int i = 0; i < 16; ++i) {
            W[i] = (static_cast<uint32_t>(data[i * 4]) << 24) |
                   (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(data[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(data[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            W[i] = theta1(W[i - 2]) + W[i - 7] + theta0(W[i - 15]) + W[i - 16];
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t T1 = h + sig1(e) + choose(e, f, g) + K[i] + W[i];
            uint32_t T2 = sig0(a) + majority(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    void sha256Update(Sha256Context* ctx, const uint8_t* data, size_t len) {
        size_t bufferIndex = ctx->count % 64;
        ctx->count += len;

        size_t partLen = 64 - bufferIndex;
        size_t i = 0;

        if (len >= partLen) {
            memcpy(&ctx->buffer[bufferIndex], data, partLen);
            sha256Transform(ctx->state, ctx->buffer);
            for (i = partLen; i + 63 < len; i += 64) {
                sha256Transform(ctx->state, &data[i]);
            }
            bufferIndex = 0;
        }
        memcpy(&ctx->buffer[bufferIndex], &data[i], len - i);
    }

    void sha256Final(Sha256Context* ctx, uint8_t digest[32]) {
        uint8_t bits[8];
        uint64_t bitCount = ctx->count * 8;
        for (int i = 0; i < 8; ++i) {
            bits[i] = static_cast<uint8_t>((bitCount >> ((7 - i) * 8)) & 0xFF);
        }

        size_t bufferIndex = ctx->count % 64;
        size_t padLen = (bufferIndex < 56) ? (56 - bufferIndex) : (120 - bufferIndex);
        uint8_t padding[64] = {0x80};

        sha256Update(ctx, padding, padLen);
        sha256Update(ctx, bits, 8);

        for (int i = 0; i < 8; ++i) {
            digest[i * 4] = static_cast<uint8_t>((ctx->state[i] >> 24) & 0xFF);
            digest[i * 4 + 1] = static_cast<uint8_t>((ctx->state[i] >> 16) & 0xFF);
            digest[i * 4 + 2] = static_cast<uint8_t>((ctx->state[i] >> 8) & 0xFF);
            digest[i * 4 + 3] = static_cast<uint8_t>(ctx->state[i] & 0xFF);
        }
    }
}

string Sha256::calculateFileHash(const string& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) return "";

    Sha256Context ctx;
    sha256Init(&ctx);

    vector<uint8_t> buffer(64 * 1024); // 64 KB stream buffer
    while (file.good()) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            sha256Update(&ctx, buffer.data(), static_cast<size_t>(bytesRead));
        }
    }

    uint8_t digest[32];
    sha256Final(&ctx, digest);

    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; ++i) {
        ss << setw(2) << static_cast<int>(digest[i]);
    }
    return ss.str();
}

string Sha256::calculateStringHash(const string& data) {
    Sha256Context ctx;
    sha256Init(&ctx);
    sha256Update(&ctx, reinterpret_cast<const uint8_t*>(data.data()), data.size());

    uint8_t digest[32];
    sha256Final(&ctx, digest);

    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; ++i) {
        ss << setw(2) << static_cast<int>(digest[i]);
    }
    return ss.str();
}
