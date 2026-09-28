#include <cmath>

#include "physics/rigid.hpp"
#include "test.hpp"

using namespace spill;

TEST("disc comes to rest on a floor") {
    RigidBodies bodies;
    bodies.items.push_back(makeDisc({50, 0}, 10.0f, 5.0f));
    const std::vector<Capsule> floor{{{-100, 100}, {200, 100}, 4.0f}};
    for (int i = 0; i < 600; ++i) {
        bodies.integrate(1.0f / 120.0f, {0, 900}, 0.0f, 0.0f);
        bodies.collideStatic(floor);
    }
    const Body& b = bodies.items[0];
    CHECK_NEAR(b.pos.y, 100.0 - 4.0 - 10.0, 0.5);
    CHECK(std::fabs(b.vel.y) < 10.0f);
}

TEST("disc rolls down a slope instead of sliding") {
    RigidBodies bodies;
    Body disc = makeDisc({0, -20}, 10.0f, 5.0f);
    disc.friction = 0.8f;
    bodies.items.push_back(disc);
    const std::vector<Capsule> slope{{{-50, -20}, {400, 200}, 2.0f}};
    for (int i = 0; i < 120; ++i) {
        bodies.integrate(1.0f / 120.0f, {0, 900}, 0.0f, 0.0f);
        bodies.collideStatic(slope);
    }
    const Body& b = bodies.items[0];
    CHECK(b.vel.x > 0.0f);
    // Rolling to the right in screen space (y down) is clockwise, positive here.
    CHECK(b.angVel > 0.0f);
    CHECK_NEAR(b.angVel * b.radius, length(b.vel), length(b.vel) * 0.25);
}

TEST("equal discs exchange velocity in an elastic head-on hit") {
    Body a = makeDisc({0, 0}, 5.0f, 1.0f);
    Body b = makeDisc({9, 0}, 5.0f, 1.0f);
    a.vel = {10, 0};
    a.restitution = b.restitution = 1.0f;
    a.friction = b.friction = 0.0f;
    CHECK(collideDiscDisc(a, b));
    CHECK_NEAR(a.vel.x, 0.0, 1e-4);
    CHECK_NEAR(b.vel.x, 10.0, 1e-4);
    CHECK(length(a.pos - b.pos) >= 10.0f - 1e-4f);
}

TEST("momentum is conserved between discs of different mass") {
    Body a = makeDisc({0, 0}, 5.0f, 3.0f);
    Body b = makeDisc({8, 1}, 5.0f, 1.0f);
    a.vel = {6, 0};
    b.vel = {-2, 1};
    const Vec2 before = a.vel * a.mass() + b.vel * b.mass();
    collideDiscDisc(a, b);
    const Vec2 after = a.vel * a.mass() + b.vel * b.mass();
    CHECK_NEAR(before.x, after.x, 1e-3);
    CHECK_NEAR(before.y, after.y, 1e-3);
}
