#include "auth.hpp"

#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

namespace cricpulse {

namespace {

// Standard SHA-256 implementation
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

std::vector<uint8_t> sha256Bytes(const std::vector<uint8_t>& data) {
    uint32_t H[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    std::vector<uint8_t> msg = data;
    uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8;
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xff));
    }

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t W[64];
        for (int i = 0; i < 16; ++i) {
            W[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            W[i] = theta1(W[i - 2]) + W[i - 7] + theta0(W[i - 15]) + W[i - 16];
        }

        uint32_t a = H[0];
        uint32_t b = H[1];
        uint32_t c = H[2];
        uint32_t d = H[3];
        uint32_t e = H[4];
        uint32_t f = H[5];
        uint32_t g = H[6];
        uint32_t h = H[7];

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

        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
        H[5] += f;
        H[6] += g;
        H[7] += h;
    }

    std::vector<uint8_t> out(32);
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = static_cast<uint8_t>((H[i] >> 24) & 0xff);
        out[i * 4 + 1] = static_cast<uint8_t>((H[i] >> 16) & 0xff);
        out[i * 4 + 2] = static_cast<uint8_t>((H[i] >> 8) & 0xff);
        out[i * 4 + 3] = static_cast<uint8_t>(H[i] & 0xff);
    }
    return out;
}

std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t b : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace

std::string computeSha256Hex(const std::string& input) {
    std::vector<uint8_t> data(input.begin(), input.end());
    return bytesToHex(sha256Bytes(data));
}

std::string computeHmacHex(const std::string& secret, const std::string& data) {
    std::vector<uint8_t> key(secret.begin(), secret.end());
    if (key.size() > 64) {
        key = sha256Bytes(key);
    }
    key.resize(64, 0x00);

    std::vector<uint8_t> k_ipad(64);
    std::vector<uint8_t> k_opad(64);
    for (size_t i = 0; i < 64; ++i) {
        k_ipad[i] = key[i] ^ 0x36;
        k_opad[i] = key[i] ^ 0x5c;
    }

    std::vector<uint8_t> innerData = k_ipad;
    innerData.insert(innerData.end(), data.begin(), data.end());
    std::vector<uint8_t> innerHash = sha256Bytes(innerData);

    std::vector<uint8_t> outerData = k_opad;
    outerData.insert(outerData.end(), innerHash.begin(), innerHash.end());
    return bytesToHex(sha256Bytes(outerData));
}

bool isSessionExpired(const PlayerSession& session, int64_t currentTimestampMs) {
    int64_t currentTimestampSec = currentTimestampMs / 1000;
    return currentTimestampSec >= session.expiresAtMs;
}

bool isSessionValid(const PlayerSession& session, int64_t currentTimestampMs, const std::string& secret) {
    std::string payload = session.playerId + ":" + session.role + ":" + std::to_string(session.expiresAtMs);
    std::string expectedSig = computeHmacHex(secret, payload);
    if (expectedSig != session.signature) {
        return false;
    }
    return !isSessionExpired(session, currentTimestampMs);
}

PlayerSession createPlayerSession(
    const std::string& playerId,
    const std::string& playerName,
    const std::string& role,
    int64_t nowMs,
    int64_t durationMs,
    const std::string& secret
) {
    PlayerSession session;
    session.playerId = playerId;
    session.playerName = playerName;
    session.role = role;
    session.issuedAtMs = nowMs;
    session.expiresAtMs = nowMs + durationMs;
    std::string payload = session.playerId + ":" + session.role + ":" + std::to_string(session.expiresAtMs);
    session.signature = computeHmacHex(secret, payload);
    return session;
}

std::string serializeSessionToken(const PlayerSession& session) {
    return session.playerId + "|" +
           session.playerName + "|" +
           session.role + "|" +
           std::to_string(session.issuedAtMs) + "|" +
           std::to_string(session.expiresAtMs) + "|" +
           session.signature;
}

PlayerSession deserializeSessionToken(const std::string& token) {
    PlayerSession session;
    std::vector<std::string> parts;
    std::stringstream ss(token);
    std::string segment;
    while (std::getline(ss, segment, '|')) {
        parts.push_back(segment);
    }
    if (parts.size() == 6) {
        session.playerId = parts[0];
        session.playerName = parts[1];
        session.role = parts[2];
        try {
            session.issuedAtMs = std::stoll(parts[3]);
            session.expiresAtMs = std::stoll(parts[4]);
        } catch (...) {
            session.issuedAtMs = 0;
            session.expiresAtMs = 0;
        }
        session.signature = parts[5];
    }
    return session;
}

} // namespace cricpulse
