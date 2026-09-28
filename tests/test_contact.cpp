#include "physics/contact.hpp"
#include "test.hpp"

using namespace spill;

TEST("particle is pushed out to the capsule surface") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    Vec2 prev{50, -20}, pos{50, -3};
    CHECK(resolveParticleCapsule(pos, prev, 2.0f, wall, 0.0f));
    CHECK_NEAR(pos.y, -7.0, 1e-4);
    CHECK_NEAR(pos.x, 50.0, 1e-4);
}

TEST("velocity into the wall is removed, sliding is kept") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    Vec2 prev{40, -10}, pos{45, -4};
    resolveParticleCapsule(pos, prev, 2.0f, wall, 0.0f);
    const Vec2 v = pos - prev;
    CHECK(v.y >= -1e-5f);
    CHECK_NEAR(v.x, 5.0, 1e-4);
}

TEST("friction reduces the sliding speed") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    Vec2 prev{40, -7}, pos{50, -6};
    resolveParticleCapsule(pos, prev, 2.0f, wall, 0.5f);
    CHECK_NEAR((pos - prev).x, 5.0, 1e-4);
}

TEST("fast particle cannot tunnel through a thin wall") {
    const Capsule wall{{0, 0}, {100, 0}, 1.0f};
    Vec2 prev{50, -10}, pos{50, 10};  // jumped straight across
    CHECK(resolveParticleCapsule(pos, prev, 1.0f, wall, 0.0f));
    CHECK(pos.y < 0.0f);
}

TEST("particles clear of the wall are untouched") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    Vec2 prev{50, -30}, pos{50, -20};
    CHECK(!resolveParticleCapsule(pos, prev, 2.0f, wall, 0.0f));
    CHECK_NEAR(pos.y, -20.0, 1e-6);
}

TEST("rounded end caps push radially") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    Vec2 prev{110, 0}, pos{104, 3};
    resolveParticleCapsule(pos, prev, 0.0f, wall, 0.0f);
    CHECK_NEAR(length(pos - Vec2{100, 0}), 5.0, 1e-4);
}
