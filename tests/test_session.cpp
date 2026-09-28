#include "game/session.hpp"
#include "test.hpp"

using namespace spill;

namespace {

// A funnel pouring into a cup directly below it.
Level cupLevel() {
    const ParseResult r = parseLevel(
        "name Cup\n"
        "ink 300\n"
        "target 60\n"
        "par 8\n"
        "emitter 640 100  0 1  200 150\n"
        "chain 6  600 500  600 640  680 640  680 500\n"
        "goal 606 520 68 114\n"
        "nodraw 0 0 200 200\n",
        "cup");
    return *r.level;
}

void runFrames(Session& s, int frames) {
    for (int i = 0; i < frames; ++i) s.update();
}

}  // namespace

TEST("nothing flows until the taps are opened") {
    Session s(cupLevel());
    runFrames(s, 30);
    CHECK(s.world().fluid.size() == 0u);
    CHECK(s.state() == Session::State::Planning);
    s.release();
    runFrames(s, 30);
    CHECK(s.world().fluid.size() > 0u);
    CHECK(s.elapsed() > 0.4f);
}

TEST("water poured into the cup is counted in the goal") {
    Session s(cupLevel());
    s.release();
    runFrames(s, 240);
    CHECK(s.inGoal() > 100);
}

TEST("strokes spend ink in fixed steps") {
    Session s(cupLevel());
    CHECK(s.beginStroke({300, 300}));
    s.extendStroke({400, 300});
    s.endStroke();
    CHECK(s.strokes().size() == 1u);
    CHECK_NEAR(s.inkUsed(), 100.0, 1e-3);
    CHECK(s.strokes()[0].points.size() == 11u);
    CHECK(s.world().walls().size() == 3u + 10u);
}

TEST("ink runs out part-way through a stroke") {
    Session s(cupLevel());
    s.beginStroke({300, 300});
    s.extendStroke({300 + 1000, 300});
    s.endStroke();
    CHECK_NEAR(s.inkUsed(), 300.0, 1e-3);
    CHECK(s.inkLeft() <= 1e-3f);
    CHECK(!s.beginStroke({300, 400}));
}

TEST("strokes stop at no-draw regions") {
    Session s(cupLevel());
    CHECK(!s.beginStroke({100, 100}));
    CHECK(s.beginStroke({300, 100}));
    s.extendStroke({100, 100});
    CHECK(!s.drawing());
    CHECK(s.strokes()[0].points.back().x >= 200.0f);
}

TEST("a click without a drag leaves no stroke") {
    Session s(cupLevel());
    s.beginStroke({300, 300});
    s.extendStroke({303, 300});
    s.endStroke();
    CHECK(s.strokes().empty());
    CHECK(s.inkUsed() == 0.0f);
}

TEST("erasing a stroke removes its walls but not what it cost") {
    Session s(cupLevel());
    s.beginStroke({300, 300});
    s.extendStroke({400, 300});
    s.endStroke();
    s.beginStroke({300, 400});
    s.extendStroke({300, 450});
    s.endStroke();
    CHECK(!s.eraseAt({350, 350}));  // nowhere near either stroke
    CHECK(s.eraseAt({350, 305}));
    CHECK(s.strokes().size() == 1u);
    CHECK_NEAR(s.inkUsed(), 150.0, 1e-3);
    CHECK(s.world().walls().size() == 3u + 5u);
}

TEST("undo removes the most recent stroke") {
    Session s(cupLevel());
    s.beginStroke({300, 300});
    s.extendStroke({400, 300});
    s.endStroke();
    s.beginStroke({300, 400});
    s.extendStroke({300, 450});
    s.endStroke();
    s.undo();
    CHECK(s.strokes().size() == 1u);
    CHECK_NEAR(s.inkUsed(), 150.0, 1e-3);
    s.undo();
    s.undo();  // harmless when empty
    CHECK(s.strokes().empty());
}

TEST("filling the goal and holding it wins the level") {
    Session s(cupLevel());
    s.release();
    runFrames(s, 60 * 6);
    CHECK(s.state() == Session::State::Won);
    CHECK(s.finishTime() > 0.0f);
    CHECK(s.finishTime() < s.elapsed());
    CHECK(s.stars() == 3);  // no ink used and well inside par
}

TEST("using more than half the ink costs a star") {
    Session s(cupLevel());
    s.beginStroke({100, 400});
    s.extendStroke({300, 400});
    s.endStroke();
    s.release();
    runFrames(s, 60 * 6);
    CHECK(s.state() == Session::State::Won);
    CHECK(s.stars() == 2);
}

TEST("water that misses the goal loses once the taps run dry") {
    Level l = cupLevel();
    // Deflect the stream away from the cup with a steep level wall.
    l.walls.push_back({{560, 250}, {760, 170}, 6.0f});
    Session s(l);
    s.release();
    runFrames(s, 60 * 12);
    CHECK(s.state() == Session::State::Lost);
    CHECK(!s.lossReason().empty());
    CHECK(s.stars() == 0);
}

TEST("no drawing after the level is decided") {
    Session s(cupLevel());
    s.release();
    runFrames(s, 60 * 6);
    CHECK(s.state() == Session::State::Won);
    CHECK(!s.beginStroke({300, 300}));
}

TEST("a polyline is drawn as a single stroke") {
    Session s(cupLevel());
    s.drawPolyline({{300, 300}, {400, 300}, {400, 350}});
    CHECK(s.strokes().size() == 1u);
    CHECK_NEAR(s.inkUsed(), 150.0, 10.0);
}

TEST("water kept moving forever still ends the level") {
    Level l = cupLevel();
    l.walls.push_back({{560, 250}, {760, 170}, 6.0f});  // miss the cup
    // A floor and a fan that keeps the spilled water churning.
    l.walls.push_back({{0, 700}, {1280, 700}, 6.0f});
    l.walls.push_back({{0, 700}, {0, 300}, 6.0f});
    l.walls.push_back({{1280, 700}, {1280, 300}, 6.0f});
    l.zones.push_back({{{0, 560}, {1280, 700}}, {0.0f, -1400.0f}});
    Session s(l);
    s.release();
    for (int i = 0; i < 60 * 30 && s.state() == Session::State::Flowing; ++i) s.update();
    CHECK(s.state() == Session::State::Lost);
}

TEST("strokes cannot be drawn through a ball") {
    Level l = cupLevel();
    l.balls.push_back({{400, 300}, 20.0f, 0.5f});
    Session s(l);
    CHECK(!s.beginStroke({400, 300}));
    CHECK(s.beginStroke({300, 300}));
    s.extendStroke({500, 300});
    CHECK(!s.drawing());
    CHECK(s.strokes()[0].points.back().x < 400.0f - 20.0f);
}

TEST("a goal riding on a slide is counted where it is now") {
    const ParseResult r = parseLevel(
        "ink 300\ntarget 10\n"
        "emitter 640 100  0 1  200 150\n"
        "slide 400 0 4\n"
        "  chain 6  600 500  600 640  680 640  680 500\n"
        "  goal 606 520 68 114\n"
        "end\n",
        "moving");
    Session s(*r.level);
    for (int i = 0; i < 120; ++i) s.update();  // 2 s: halfway through the period
    CHECK_NEAR(s.goalArea(0).min.x, 606.0 + 400.0, 1e-2);
    CHECK(s.world().walls().empty());
    CHECK(s.world().movers.size() == 1u);
}

TEST("hold time comes from the level") {
    Level l = cupLevel();
    l.hold = 3.0f;
    Session s(l);
    s.release();
    int framesFull = 0;
    while (s.state() == Session::State::Flowing && framesFull < 60 * 20) {
        s.update();
        if (s.inGoal() >= l.target) ++framesFull;
    }
    CHECK(s.state() == Session::State::Won);
    CHECK(framesFull >= 179);
}

TEST("no erasing or undo once the taps are open") {
    Session s(cupLevel());
    s.beginStroke({300, 300});
    s.extendStroke({400, 300});
    s.endStroke();
    s.release();
    CHECK(!s.eraseAt({350, 300}));
    s.undo();
    CHECK(s.strokes().size() == 1u);
    // Drawing more is still allowed, while the ink lasts.
    CHECK(s.beginStroke({300, 400}));
}
