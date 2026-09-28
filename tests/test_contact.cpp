#include "physics/contact.hpp"
#include "physics/rigid.hpp"
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

TEST("particle hitting a disc transfers momentum to it") {
    Body disc = makeDisc({0, 0}, 10.0f, 4.0f);
    const float dt = 1.0f / 100.0f;
    Vec2 prev{-13, 0}, pos{-11, 0};  // moving right at 200 u/s, overlapping
    const Vec2 before = (pos - prev) / dt * 1.0f + disc.vel * disc.mass();
    CHECK(resolveParticleDisc(pos, prev, 2.0f, 1.0f, disc, 0.0f, dt));
    const Vec2 after = (pos - prev) / dt * 1.0f + disc.vel * disc.mass();
    CHECK(disc.vel.x > 0.0f);
    CHECK_NEAR(before.x, after.x, 1e-2);
    CHECK_NEAR(length(pos - disc.pos), 12.0, 1e-4);
}

TEST("particles resting on a moving disc are carried along") {
    Body disc = makeDisc({0, 0}, 10.0f, 1000.0f);
    disc.vel = {0, -50};  // rising
    const float dt = 1.0f / 100.0f;
    Vec2 prev{0, -12}, pos{0, -11.5f};
    resolveParticleDisc(pos, prev, 2.0f, 1.0f, disc, 0.0f, dt);
    CHECK((pos - prev).y / dt < -45.0f);
}

TEST("a moving wall carries a resting particle with it") {
    const Capsule wall{{0, 0}, {100, 0}, 5.0f};
    // The wall rose 3 units this step and the particle, sitting on it, did not.
    Vec2 prev{50, -6}, pos{50, -6};
    CHECK(resolveParticleCapsule(pos, prev, 2.0f, wall, 0.0f, {0, -3}));
    CHECK_NEAR((pos - prev).y, -3.0, 1e-4);
}

TEST("a sweeping wall does not pass through a particle") {
    // The wall moved 12 up this step and now sits above a particle that was
    // above it before: the particle belongs on top.
    const Capsule wall{{0, -12}, {100, -12}, 2.0f};
    Vec2 prev{50, -8}, pos{50, -8};
    resolveParticleCapsule(pos, prev, 1.0f, wall, 0.0f, {0, -12});
    CHECK(pos.y < -12.0f);
}
