#include <algorithm>
#include <cmath>

#include "physics/world.hpp"
#include "test.hpp"

using namespace spill;

namespace {

// An open-topped tank 400 wide with its floor at y = 600.
World makeTank() {
    World w;
    w.configure({{0, 0}, {1280, 720}}, WorldParams{}, FluidParams{});
    w.setWalls({{{200, 600}, {600, 600}, 6.0f},
                {{200, 600}, {200, 100}, 6.0f},
                {{600, 600}, {600, 100}, 6.0f}});
    return w;
}

void fillBlock(World& w, Vec2 origin, int cols, int rows, float spacing = 7.0f) {
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            w.fluid.add(origin + Vec2{x * spacing, -y * spacing}, {});
        }
    }
}

void run(World& w, float seconds) {
    const int frames = static_cast<int>(seconds * 60.0f);
    for (int i = 0; i < frames; ++i) w.step(1.0f / 60.0f);
}

float surfaceHeight(const World& w) {
    float top = 1e9f;
    for (const Vec2& p : w.fluid.pos) top = std::min(top, p.y);
    return top;
}

}  // namespace

TEST("dam break settles without losing or exploding particles") {
    World w = makeTank();
    fillBlock(w, {215, 590}, 20, 40);
    run(w, 10.0f);
    CHECK(w.fluid.size() == 800u);
    CHECK(w.escaped == 0);
    float maxSpeed = 0.0f;
    for (size_t i = 0; i < w.fluid.size(); ++i) {
        const Vec2 p = w.fluid.pos[i];
        CHECK(std::isfinite(p.x) && std::isfinite(p.y));
        CHECK(p.x > 200.0f && p.x < 600.0f && p.y < 600.0f);
        maxSpeed = std::max(maxSpeed, length(w.fluid.vel[i]));
    }
    CHECK(maxSpeed < 150.0f);
}

TEST("settled fluid is close to incompressible") {
    // Doubling the amount of fluid in the tank should double the pool depth.
    // A compressible fluid would pack the deeper pool tighter at the bottom.
    auto depthOf = [](int rows) {
        World w = makeTank();
        fillBlock(w, {215, 590}, 50, rows);
        run(w, 8.0f);
        return 594.0f - surfaceHeight(w);
    };
    const float shallow = depthOf(15);
    const float deep = depthOf(30);
    CHECK(deep / shallow > 1.8f);
    CHECK(deep / shallow < 2.2f);
}

TEST("drains swallow fluid and count it") {
    World w = makeTank();
    w.drains.push_back({{{200, 560}, {600, 610}}});
    fillBlock(w, {215, 400}, 10, 10);
    run(w, 3.0f);
    CHECK(w.fluid.size() == 0u);
    CHECK(w.drained == 100);
}

TEST("particles falling out of the world are counted as escaped") {
    World w;
    w.configure({{0, 0}, {400, 300}}, WorldParams{}, FluidParams{});
    fillBlock(w, {100, 200}, 5, 5);
    run(w, 2.0f);
    CHECK(w.fluid.size() == 0u);
    CHECK(w.escaped == 25);
}

TEST("force zones push fluid sideways") {
    World w = makeTank();
    w.zones.push_back({{{200, 100}, {600, 600}}, {1500.0f, 0.0f}});
    fillBlock(w, {215, 590}, 10, 10);
    run(w, 2.0f);
    float meanX = 0.0f;
    for (const Vec2& p : w.fluid.pos) meanX += p.x;
    meanX /= static_cast<float>(w.fluid.size());
    CHECK(meanX > 500.0f);
}

TEST("light discs float and heavy discs sink") {
    World w = makeTank();
    fillBlock(w, {215, 590}, 55, 30);
    run(w, 3.0f);
    const float surface = surfaceHeight(w);

    w.addDisc({300, 300}, 18.0f, 0.4f);
    w.addDisc({500, 300}, 18.0f, 3.0f);
    run(w, 5.0f);
    const Body& light = w.bodies.items[0];
    const Body& heavy = w.bodies.items[1];
    CHECK(light.pos.y < surface + 18.0f);  // riding at the surface
    CHECK(heavy.pos.y > 594.0f - 30.0f);   // on the bottom
}

TEST("a sliding paddle sweeps water across a tank") {
    World w = makeTank();
    fillBlock(w, {215, 590}, 20, 8);
    run(w, 2.0f);
    Motion m;
    m.kind = Motion::Kind::Slide;
    m.offset = {300, 0};
    m.period = 4.0f;
    m.phase = 0.5f;  // movers run on world time: at rest now (t = 2), fully across at t = 4
    w.movers.emplace_back(std::vector<Capsule>{{{230, 600}, {230, 450}, 5.0f}}, m);
    run(w, 2.0f);
    float meanX = 0.0f;
    for (const Vec2& p : w.fluid.pos) meanX += p.x;
    meanX /= static_cast<float>(w.fluid.size());
    CHECK(meanX > 540.0f);
    CHECK(w.escaped == 0);
}

TEST("a triggered shift moves when water arrives") {
    World w = makeTank();
    Motion m;
    m.kind = Motion::Kind::Shift;
    m.offset = {0, -200};
    m.duration = 0.5f;
    m.trigger = {{200, 500}, {600, 600}};
    w.movers.emplace_back(std::vector<Capsule>{{{700, 300}, {800, 300}, 5.0f}}, m);
    run(w, 1.0f);
    CHECK(!w.movers[0].triggered());
    fillBlock(w, {300, 590}, 5, 5);
    run(w, 1.0f);
    CHECK(w.movers[0].triggered());
    CHECK_NEAR(w.movers[0].capsules()[0].a.y, 100.0, 1e-2);
}

TEST("a rising platform carries a ball") {
    World w;
    w.configure({{0, 0}, {1280, 720}}, WorldParams{}, FluidParams{});
    Motion m;
    m.kind = Motion::Kind::Slide;
    m.offset = {0, -200};
    m.period = 4.0f;
    w.movers.emplace_back(std::vector<Capsule>{{{500, 600}, {700, 600}, 6.0f}}, m);
    w.addDisc({600, 570}, 20.0f, 2.0f);
    run(w, 2.0f);  // the platform is at its highest now
    CHECK(w.bodies.items[0].pos.y < 600.0f - 200.0f);
}
