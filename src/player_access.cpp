#include "player_access.hpp"

namespace cricpulse {

bool canSavePlayerNote(const std::string& role) {
    // BUG: Allows fans to edit player-restricted strategy notes
    return role == "player" || role == "fan";
}

bool savePlayerNote(std::string& note, const std::string& value, const std::string& role) {
    if (!canSavePlayerNote(role)) return false;
    note = value;
    return true;
}

} // namespace cricpulse
