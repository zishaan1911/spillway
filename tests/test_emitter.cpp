#include <cmath>

#include "game/emitter.hpp"
#include "test.hpp"

using namespace spill;

namespace {
EmitterDef nozzle(int total) {
    EmitterDef d;
    d.pos = {100, 100};
    d.dir = {0, 1};
    d.speed = 120.0f;
    d.total = total;
    d.width = 24.0f;
    return d;
}
}  // namespace

TEST("emitter lays down rows across its nozzle") {
    Fluid fluid;
    Emitter e(nozzle(1000), 6.0f);
    CHECK(e.perRow() == 4);
    CHECK(e.update(0.0f, fluid) == 4);  // first row is immediate
    for (size_t i = 0; i < fluid.size(); ++i) {
        CHECK_NEAR(fluid.pos[i].y, 100.0, 1e-4);
        CHECK_NEAR(fluid.vel[i].y, 120.0, 1e-4);
    }
}

TEST("emitter flow matches speed and width") {
    Fluid fluid;
    Emitter e(nozzle(100000), 6.0f);
    for (int i = 0; i < 60; ++i) e.update(1.0f / 60.0f, fluid);
    // 4 per row, a row every 6 units at 120 units/s: 80 per second.
    CHECK_NEAR(e.ratePerSecond(), 80.0, 1e-3);
    CHECK_NEAR(static_cast<double>(fluid.size()), 84.0, 4.0);
}

TEST("emitter stops at its total") {
    Fluid fluid;
    Emitter e(nozzle(10), 6.0f);
    for (int i = 0; i < 600; ++i) e.update(1.0f / 60.0f, fluid);
    CHECK(fluid.size() == 10u);
    CHECK(e.empty());
    CHECK(e.remaining() == 0);
}

TEST("alternate rows are staggered by half a spacing") {
    Fluid fluid;
    Emitter e(nozzle(1000), 6.0f);
    e.update(0.0f, fluid);
    e.update(6.0f / 120.0f, fluid);  // exactly one spacing of travel
    CHECK(fluid.size() == 8u);
    CHECK_NEAR(std::fabs(fluid.pos[4].x - fluid.pos[0].x), 3.0, 1e-4);
}
