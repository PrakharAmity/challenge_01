#pragma once

#include <cstdint>
#include <string>

namespace cricpulse {

struct PlayerSession {
    std::string playerId;
    std::string playerName;
    std::string role;
    int64_t issuedAtMs{0};
    int64_t expiresAtMs{0};
    std::string signature;
};

std::string computeSha256Hex(const std::string& input);
std::string computeHmacHex(const std::string& secret, const std::string& data);

bool isSessionExpired(const PlayerSession& session, int64_t currentTimestampMs);
bool isSessionValid(const PlayerSession& session, int64_t currentTimestampMs, const std::string& secret);

PlayerSession createPlayerSession(
    const std::string& playerId,
    const std::string& playerName,
    const std::string& role,
    int64_t nowMs,
    int64_t durationMs,
    const std::string& secret
);

std::string serializeSessionToken(const PlayerSession& session);
PlayerSession deserializeSessionToken(const std::string& token);

} // namespace cricpulse
