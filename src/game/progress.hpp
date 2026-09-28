#pragma once

#include <map>
#include <string>

namespace spill {

struct LevelRecord {
    int stars = 0;
    float bestTime = 0.0f;  // 0 means never finished
};

// Best result per level, stored as plain text: one "id stars time" per line.
class Progress {
public:
    // Keeps the better of the stored and new result. Returns true if anything
    // improved.
    bool record(const std::string& levelId, int stars, float time);
    LevelRecord get(const std::string& levelId) const;
    int totalStars() const;

    std::string serialize() const;
    static Progress parse(const std::string& text);

    bool save(const std::string& path) const;
    static Progress load(const std::string& path);  // empty if missing

private:
    std::map<std::string, LevelRecord> records_;
};

}  // namespace spill
