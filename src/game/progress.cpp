#include "game/progress.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace spill {

bool Progress::record(const std::string& levelId, int stars, float time) {
    LevelRecord& r = records_[levelId];
    bool improved = false;
    if (stars > r.stars) {
        r.stars = stars;
        improved = true;
    }
    if (stars > 0 && (r.bestTime == 0.0f || time < r.bestTime)) {
        r.bestTime = time;
        improved = true;
    }
    return improved;
}

void Progress::addSpill(const std::string& levelId) { ++records_[levelId].spills; }

LevelRecord Progress::get(const std::string& levelId) const {
    const auto it = records_.find(levelId);
    return it == records_.end() ? LevelRecord{} : it->second;
}

int Progress::totalStars() const {
    int total = 0;
    for (const auto& [id, r] : records_) total += r.stars;
    return total;
}

int Progress::totalSpills() const {
    int total = 0;
    for (const auto& [id, r] : records_) total += r.spills;
    return total;
}

std::string Progress::serialize() const {
    std::ostringstream out;
    for (const auto& [id, r] : records_) {
        out << id << ' ' << r.stars << ' ' << r.bestTime << ' ' << r.spills << '\n';
    }
    return out.str();
}

Progress Progress::parse(const std::string& text) {
    Progress p;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream fields(line);
        std::string id;
        LevelRecord r;
        if (!(fields >> id >> r.stars >> r.bestTime)) continue;  // skip junk lines
        if (!(fields >> r.spills)) r.spills = 0;                 // older saves had no count
        r.stars = std::clamp(r.stars, 0, 3);
        r.spills = std::max(r.spills, 0);
        p.records_[id] = r;
    }
    return p;
}

bool Progress::save(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << serialize();
    return static_cast<bool>(out);
}

Progress Progress::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::stringstream buf;
    buf << in.rdbuf();
    return parse(buf.str());
}

}  // namespace spill
