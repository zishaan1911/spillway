// Plays every shipped level twice: once untouched, which must not win, and
// once with its reference solution drawn, which must. This keeps the levels
// honest when the physics is retuned.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "game/session.hpp"
#include "test.hpp"

using namespace spill;
namespace fs = std::filesystem;

namespace {

std::vector<Level> shippedLevels() {
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(SPILLWAY_LEVEL_DIR)) {
        if (e.path().extension() == ".lvl") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    std::vector<Level> levels;
    for (const fs::path& f : files) {
        std::ifstream in(f);
        std::stringstream buf;
        buf << in.rdbuf();
        ParseResult r = parseLevel(buf.str(), f.stem().string());
        if (!r.level) {
            test::fail(__FILE__, __LINE__, f.filename().string() + ": " + r.error);
            continue;
        }
        levels.push_back(std::move(*r.level));
    }
    return levels;
}

Session::State play(const Level& level, bool solved) {
    Session s(level);
    if (solved) {
        for (const auto& stroke : level.solution) s.drawPolyline(stroke);
    }
    s.release();
    for (int frame = 0; frame < 60 * 60; ++frame) {
        s.update();
        if (s.state() == Session::State::Won || s.state() == Session::State::Lost) break;
    }
    return s.state();
}

}  // namespace

TEST("every level parses and has a solution") {
    const auto levels = shippedLevels();
    CHECK(levels.size() >= 1u);
    for (const Level& l : levels) {
        if (l.solution.empty()) test::fail(__FILE__, __LINE__, l.id + " has no solution");
    }
}

TEST("no level can be won without drawing") {
    for (const Level& l : shippedLevels()) {
        if (play(l, false) == Session::State::Won) {
            test::fail(__FILE__, __LINE__, l.id + " wins with no strokes");
        }
    }
}

TEST("every level's solution wins") {
    for (const Level& l : shippedLevels()) {
        if (play(l, true) != Session::State::Won) {
            test::fail(__FILE__, __LINE__, l.id + " solution does not win");
        }
    }
}
