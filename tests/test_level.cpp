#include "game/level.hpp"
#include "test.hpp"

using namespace spill;

namespace {
const char* kMinimal =
    "# a comment\n"
    "name  Test Level\n"
    "hint  Pour it in.\n"
    "ink 400\n"
    "target 100\n"
    "par 12.5\n"
    "emitter 100 50  0 1  150 300\n"
    "goal 500 600 80 60\n";
}

TEST("parses a minimal level") {
    const ParseResult r = parseLevel(kMinimal, "t");
    CHECK(r.level.has_value());
    const Level& l = *r.level;
    CHECK(l.id == "t");
    CHECK(l.name == "Test Level");
    CHECK(l.hint == "Pour it in.");
    CHECK_NEAR(l.ink, 400.0, 1e-6);
    CHECK(l.target == 100);
    CHECK_NEAR(l.par, 12.5, 1e-6);
    CHECK(l.emitters.size() == 1);
    CHECK(l.emitters[0].total == 300);
    CHECK_NEAR(l.emitters[0].width, 24.0, 1e-6);
    CHECK_NEAR(l.goals[0].area.max.x, 580.0, 1e-6);
}

TEST("chain expands into connected walls") {
    const std::string text = std::string(kMinimal) + "chain 4  0 0  10 0  10 10  0 10\n";
    const ParseResult r = parseLevel(text, "t");
    CHECK(r.level.has_value());
    CHECK(r.level->walls.size() == 3);
    CHECK_NEAR(r.level->walls[2].b.x, 0.0, 1e-6);
    CHECK_NEAR(r.level->walls[2].radius, 4.0, 1e-6);
}

TEST("optional fields take defaults") {
    const std::string text = std::string(kMinimal) + "wall 0 0 10 10\nball 5 5 12\n";
    const ParseResult r = parseLevel(text, "t");
    CHECK(r.level.has_value());
    CHECK_NEAR(r.level->walls[0].radius, 6.0, 1e-6);
    CHECK_NEAR(r.level->balls[0].density, 0.5, 1e-6);
}

TEST("trailing comments and windows line endings are fine") {
    const ParseResult r = parseLevel("emitter 1 2 0 1 3 5 # nozzle\r\ngoal 0 0 1 1\r\ntarget 1\r\n", "t");
    CHECK(r.level.has_value());
}

TEST("reports the line of an unknown keyword") {
    const ParseResult r = parseLevel(std::string(kMinimal) + "wal 0 0 1 1\n", "t");
    CHECK(!r.level.has_value());
    CHECK(r.error.rfind("line 9:", 0) == 0);
}

TEST("rejects bad numbers and wrong arity") {
    CHECK(!parseLevel(std::string(kMinimal) + "wall 0 0 x 1\n", "t").level);
    CHECK(!parseLevel(std::string(kMinimal) + "goal 1 2 3\n", "t").level);
    CHECK(!parseLevel(std::string(kMinimal) + "chain 4 0 0\n", "t").level);
}

TEST("rejects levels that cannot be won") {
    CHECK(!parseLevel("goal 0 0 1 1\n", "t").level);             // no emitter
    CHECK(!parseLevel("emitter 1 2 0 1 3 5\n", "t").level);      // no goal
    CHECK(!parseLevel("emitter 1 2 0 1 3 5\ngoal 0 0 1 1\ntarget 9\n", "t").level);
}

TEST("solution lines become reference strokes") {
    const ParseResult r =
        parseLevel(std::string(kMinimal) + "solution 0 0 10 10 20 0\nsolution 5 5 6 6\n", "t");
    CHECK(r.level.has_value());
    CHECK(r.level->solution.size() == 2u);
    CHECK(r.level->solution[0].size() == 3u);
    CHECK(!parseLevel(std::string(kMinimal) + "solution 1 2 3\n", "t").level);
}
