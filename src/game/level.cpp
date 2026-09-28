#include "game/level.hpp"

#include <sstream>

namespace spill {

int Level::totalFluid() const {
    int sum = 0;
    for (const EmitterDef& e : emitters) sum += e.total;
    return sum;
}

namespace {

// Reads floats from the rest of a line; fails on anything that isn't one.
bool readFloats(std::istringstream& in, std::vector<float>& out) {
    out.clear();
    std::string token;
    while (in >> token) {
        if (token[0] == '#') break;
        try {
            size_t used = 0;
            out.push_back(std::stof(token, &used));
            if (used != token.size()) return false;
        } catch (...) {
            return false;
        }
    }
    return true;
}

std::string restOfLine(std::istringstream& in) {
    std::string rest;
    std::getline(in, rest);
    const size_t first = rest.find_first_not_of(" \t");
    return first == std::string::npos ? std::string{} : rest.substr(first);
}

Aabb rect(const std::vector<float>& v, size_t at) {
    return {{v[at], v[at + 1]}, {v[at] + v[at + 2], v[at + 1] + v[at + 3]}};
}

}  // namespace

ParseResult parseLevel(const std::string& text, const std::string& id) {
    Level level;
    level.id = id;
    std::istringstream lines(text);
    std::string line;
    int lineNo = 0;
    std::vector<float> v;

    auto fail = [&](const std::string& msg) {
        return ParseResult{std::nullopt, "line " + std::to_string(lineNo) + ": " + msg};
    };

    while (std::getline(lines, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream in(line);
        std::string key;
        if (!(in >> key) || key[0] == '#') continue;

        if (key == "name") {
            level.name = restOfLine(in);
            continue;
        }
        if (key == "hint") {
            level.hint = restOfLine(in);
            continue;
        }

        if (!readFloats(in, v)) return fail("expected numbers after '" + key + "'");
        auto need = [&](size_t lo, size_t hi) { return v.size() >= lo && v.size() <= hi; };

        if (key == "ink") {
            if (!need(1, 1)) return fail("ink takes 1 number");
            level.ink = v[0];
        } else if (key == "target") {
            if (!need(1, 1)) return fail("target takes 1 number");
            level.target = static_cast<int>(v[0]);
        } else if (key == "par") {
            if (!need(1, 1)) return fail("par takes 1 number");
            level.par = v[0];
        } else if (key == "emitter") {
            if (!need(7, 8)) return fail("emitter takes x y dx dy speed rate total [width]");
            EmitterDef e;
            e.pos = {v[0], v[1]};
            e.dir = normalize({v[2], v[3]});
            if (lengthSq(e.dir) == 0.0f) return fail("emitter direction is zero");
            e.speed = v[4];
            e.rate = v[5];
            e.total = static_cast<int>(v[6]);
            if (v.size() == 8) e.width = v[7];
            level.emitters.push_back(e);
        } else if (key == "wall") {
            if (!need(4, 5)) return fail("wall takes x1 y1 x2 y2 [radius]");
            level.walls.push_back({{v[0], v[1]}, {v[2], v[3]}, v.size() == 5 ? v[4] : 6.0f});
        } else if (key == "chain") {
            // chain radius x1 y1 x2 y2 ... : a connected run of walls
            if (v.size() < 5 || (v.size() - 1) % 2 != 0) {
                return fail("chain takes radius then at least two x y points");
            }
            for (size_t i = 1; i + 3 < v.size(); i += 2) {
                level.walls.push_back({{v[i], v[i + 1]}, {v[i + 2], v[i + 3]}, v[0]});
            }
        } else if (key == "goal") {
            if (!need(4, 4)) return fail("goal takes x y w h");
            level.goals.push_back({rect(v, 0)});
        } else if (key == "drain") {
            if (!need(4, 4)) return fail("drain takes x y w h");
            level.drains.push_back(rect(v, 0));
        } else if (key == "zone") {
            if (!need(6, 6)) return fail("zone takes x y w h ax ay");
            level.zones.push_back({rect(v, 0), {v[4], v[5]}});
        } else if (key == "ball") {
            if (!need(3, 4)) return fail("ball takes x y radius [density]");
            level.balls.push_back({{v[0], v[1]}, v[2], v.size() == 4 ? v[3] : 0.5f});
        } else if (key == "nodraw") {
            if (!need(4, 4)) return fail("nodraw takes x y w h");
            level.noDraw.push_back(rect(v, 0));
        } else {
            return fail("unknown keyword '" + key + "'");
        }
    }

    lineNo = 0;
    if (level.emitters.empty()) return fail("level has no emitter");
    if (level.goals.empty()) return fail("level has no goal");
    if (level.target > level.totalFluid()) return fail("target is more fluid than the emitters hold");
    return {level, {}};
}

}  // namespace spill
