#pragma once

#include <string>

namespace cricpulse {

bool canSavePlayerNote(const std::string& role);
bool savePlayerNote(std::string& note, const std::string& value, const std::string& role);

} // namespace cricpulse
