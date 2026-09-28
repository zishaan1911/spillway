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
    int group = -1;  // mover currently being defined, between a motion and `end`

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

        // Walls and goals inside a moving group belong to that mover.
        auto addWall = [&](const Capsule& c) {
            if (group >= 0) {
                level.movers[group].walls.push_back(c);
            } else {
                level.walls.push_back(c);
            }
        };
        auto beginGroup = [&](const Motion& m) {
            level.movers.push_back({m, {}});
            group = static_cast<int>(level.movers.size()) - 1;
        };

        if (group >= 0 && key != "wall" && key != "chain" && key != "goal" && key != "end") {
            return fail("only wall, chain and goal can go inside a moving group");
        }

        if (key == "ink") {
            if (!need(1, 1)) return fail("ink takes 1 number");
            level.ink = v[0];
        } else if (key == "target") {
            if (!need(1, 1)) return fail("target takes 1 number");
            level.target = static_cast<int>(v[0]);
        } else if (key == "par") {
            if (!need(1, 1)) return fail("par takes 1 number");
            level.par = v[0];
        } else if (key == "hold") {
            if (!need(1, 1)) return fail("hold takes 1 number");
            level.hold = v[0];
        } else if (key == "slide") {
            if (!need(3, 4)) return fail("slide takes dx dy period [phase]");
            Motion m;
            m.kind = Motion::Kind::Slide;
            m.offset = {v[0], v[1]};
            m.period = v[2];
            if (v.size() == 4) m.phase = v[3];
            beginGroup(m);
        } else if (key == "spin") {
            if (!need(3, 3)) return fail("spin takes px py degrees-per-second");
            Motion m;
            m.kind = Motion::Kind::Spin;
            m.pivot = {v[0], v[1]};
            m.angularSpeed = v[2] * 3.14159265f / 180.0f;
            beginGroup(m);
        } else if (key == "shift") {
            if (!need(7, 7)) return fail("shift takes dx dy seconds tx ty tw th");
            Motion m;
            m.kind = Motion::Kind::Shift;
            m.offset = {v[0], v[1]};
            m.duration = v[2];
            m.trigger = rect(v, 3);
            beginGroup(m);
        } else if (key == "end") {
            if (group < 0) return fail("end without a moving group");
            if (level.movers[group].walls.empty()) return fail("moving group has no walls");
            group = -1;
        } else if (key == "fake") {
            if (!need(4, 5)) return fail("fake takes x1 y1 x2 y2 [radius]");
            level.fakes.push_back({{v[0], v[1]}, {v[2], v[3]}, v.size() == 5 ? v[4] : 6.0f});
        } else if (key == "emitter") {
            if (!need(6, 7)) return fail("emitter takes x y dx dy speed total [width]");
            EmitterDef e;
            e.pos = {v[0], v[1]};
            e.dir = normalize({v[2], v[3]});
            if (lengthSq(e.dir) == 0.0f) return fail("emitter direction is zero");
            e.speed = v[4];
            e.total = static_cast<int>(v[5]);
            if (v.size() == 7) e.width = v[6];
            level.emitters.push_back(e);
        } else if (key == "wall") {
            if (!need(4, 5)) return fail("wall takes x1 y1 x2 y2 [radius]");
            addWall({{v[0], v[1]}, {v[2], v[3]}, v.size() == 5 ? v[4] : 6.0f});
        } else if (key == "chain") {
            // chain radius x1 y1 x2 y2 ... : a connected run of walls
            if (v.size() < 5 || (v.size() - 1) % 2 != 0) {
                return fail("chain takes radius then at least two x y points");
            }
            for (size_t i = 1; i + 3 < v.size(); i += 2) {
                addWall({{v[i], v[i + 1]}, {v[i + 2], v[i + 3]}, v[0]});
            }
        } else if (key == "goal") {
            if (!need(4, 4)) return fail("goal takes x y w h");
            if (group >= 0 && level.movers[group].motion.kind == Motion::Kind::Spin) {
                return fail("goals can ride on slides and shifts, not spins");
            }
            level.goals.push_back({rect(v, 0), group});
        } else if (key == "drain") {
            if (!need(4, 4)) return fail("drain takes x y w h");
            level.drains.push_back(rect(v, 0));
        } else if (key == "zone") {
            if (!need(6, 6)) return fail("zone takes x y w h ax ay");
            level.zones.push_back({rect(v, 0), {v[4], v[5]}});
        } else if (key == "ball") {
            if (!need(3, 4)) return fail("ball takes x y radius [density]");
            level.balls.push_back({{v[0], v[1]}, v[2], v.size() == 4 ? v[3] : 0.5f});
        } else if (key == "solution") {
            if (v.size() < 4 || v.size() % 2 != 0) return fail("solution takes at least two x y points");
            std::vector<Vec2> stroke;
            for (size_t i = 0; i + 1 < v.size(); i += 2) stroke.push_back({v[i], v[i + 1]});
            level.solution.push_back(std::move(stroke));
        } else if (key == "nodraw") {
            if (!need(4, 4)) return fail("nodraw takes x y w h");
            level.noDraw.push_back(rect(v, 0));
        } else {
            return fail("unknown keyword '" + key + "'");
        }
    }

    if (group >= 0) return fail("moving group is missing its end");
    lineNo = 0;
    if (level.emitters.empty()) return fail("level has no emitter");
    if (level.goals.empty()) return fail("level has no goal");
    if (level.target > level.totalFluid()) return fail("target is more fluid than the emitters hold");
    return {level, {}};
}

}  // namespace spill
