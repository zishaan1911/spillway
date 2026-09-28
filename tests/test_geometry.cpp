#include "physics/geometry.hpp"
#include "test.hpp"

using namespace spill;

TEST("closest point clamps to segment ends") {
    const Vec2 a{0, 0}, b{10, 0};
    const Vec2 before = closestPointOnSegment({-5, 3}, a, b);
    const Vec2 after = closestPointOnSegment({15, -2}, a, b);
    CHECK_NEAR(before.x, 0.0, 1e-6);
    CHECK_NEAR(after.x, 10.0, 1e-6);
}

TEST("closest point projects onto segment interior") {
    const Vec2 p = closestPointOnSegment({4, 7}, {0, 0}, {10, 0});
    CHECK_NEAR(p.x, 4.0, 1e-6);
    CHECK_NEAR(p.y, 0.0, 1e-6);
    CHECK_NEAR(distanceToSegment({4, 7}, {0, 0}, {10, 0}), 7.0, 1e-5);
}

TEST("degenerate segment behaves like a point") {
    CHECK_NEAR(distanceToSegment({3, 4}, {0, 0}, {0, 0}), 5.0, 1e-5);
}

TEST("capsule bounds include the radius") {
    const Capsule c{{0, 0}, {10, 5}, 2};
    const Aabb box = c.bounds();
    CHECK_NEAR(box.min.x, -2.0, 1e-6);
    CHECK_NEAR(box.max.y, 7.0, 1e-6);
    CHECK(box.contains({5, 2}));
    CHECK(!box.contains({13, 2}));
}

TEST("normalize never returns nan") {
    const Vec2 n = normalize({0, 0});
    CHECK(n.x == 0.0f && n.y == 0.0f);
    CHECK_NEAR(length(normalize({3, 4})), 1.0, 1e-6);
}
