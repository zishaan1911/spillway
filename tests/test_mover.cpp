#include "physics/mover.hpp"
#include "test.hpp"

using namespace spill;

namespace {
std::vector<Capsule> bar() { return {{{0, 0}, {100, 0}, 5.0f}}; }
}  // namespace

TEST("slide goes out and back once per period") {
    Motion m;
    m.kind = Motion::Kind::Slide;
    m.offset = {200, 0};
    m.period = 4.0f;
    Mover mover(bar(), m);
    mover.update(0.0f);
    CHECK_NEAR(mover.capsules()[0].a.x, 0.0, 1e-3);
    mover.update(2.0f);
    CHECK_NEAR(mover.capsules()[0].a.x, 200.0, 1e-3);
    mover.update(4.0f);
    CHECK_NEAR(mover.capsules()[0].a.x, 0.0, 1e-3);
}

TEST("slide velocity matches its motion") {
    Motion m;
    m.kind = Motion::Kind::Slide;
    m.offset = {0, 120};
    m.period = 3.0f;
    m.phase = 0.1f;
    Mover mover(bar(), m);
    const float t = 0.7f, h = 1e-3f;
    mover.update(t - h);
    const float y0 = mover.capsules()[0].a.y;
    mover.update(t + h);
    const float y1 = mover.capsules()[0].a.y;
    mover.update(t);
    CHECK_NEAR(mover.velocityAt({50, 0}).y, (y1 - y0) / (2 * h), 0.5);
}

TEST("spin rotates about its pivot") {
    Motion m;
    m.kind = Motion::Kind::Spin;
    m.pivot = {0, 0};
    m.angularSpeed = 3.14159265f / 2.0f;  // a quarter turn per second
    Mover mover(bar(), m);
    mover.update(1.0f);
    CHECK_NEAR(mover.capsules()[0].b.x, 0.0, 1e-3);
    CHECK_NEAR(mover.capsules()[0].b.y, 100.0, 1e-3);
    // Surface speed grows with distance from the pivot.
    CHECK_NEAR(length(mover.velocityAt({0, 100})), 100.0 * 3.14159265 / 2.0, 1e-2);
}

TEST("shift waits for its trigger, then moves once") {
    Motion m;
    m.kind = Motion::Kind::Shift;
    m.offset = {0, -80};
    m.duration = 1.0f;
    Mover mover(bar(), m);
    mover.update(5.0f);
    CHECK_NEAR(mover.translation().y, 0.0, 1e-6);
    mover.trigger(5.0f);
    mover.trigger(6.0f);  // a second trigger does not restart it
    mover.update(5.5f);
    CHECK_NEAR(mover.translation().y, -40.0, 1e-3);
    mover.update(9.0f);
    CHECK_NEAR(mover.translation().y, -80.0, 1e-3);
    CHECK_NEAR(length(mover.velocityAt({0, 0})), 0.0, 1e-6);
}
