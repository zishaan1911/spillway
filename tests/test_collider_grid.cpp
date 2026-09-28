#include <random>
#include <set>

#include "physics/collider_grid.hpp"
#include "test.hpp"

using namespace spill;

TEST("collider grid never misses a capsule within the margin") {
    std::mt19937 rng(11);
    std::uniform_real_distribution<float> ux(0.0f, 400.0f), uy(0.0f, 300.0f);
    std::vector<Capsule> caps;
    for (int i = 0; i < 40; ++i) caps.push_back({{ux(rng), uy(rng)}, {ux(rng), uy(rng)}, 4.0f});

    const float margin = 6.0f;
    ColliderGrid grid;
    grid.configure({{0, 0}, {400, 300}}, 24.0f, margin);
    grid.rebuild(caps);

    for (int k = 0; k < 2000; ++k) {
        const Vec2 p{ux(rng), uy(rng)};
        std::set<uint32_t> found;
        grid.forEachCandidate(p, [&](uint32_t i) { found.insert(i); });
        for (uint32_t i = 0; i < caps.size(); ++i) {
            if (distanceToSegment(p, caps[i].a, caps[i].b) <= caps[i].radius + margin) {
                CHECK(found.count(i) == 1);
            }
        }
    }
}

TEST("collider grid culls far-away capsules") {
    std::vector<Capsule> caps{{{0, 0}, {400, 300}, 3.0f}};
    ColliderGrid grid;
    grid.configure({{0, 0}, {400, 300}}, 20.0f, 4.0f);
    grid.rebuild(caps);
    int hits = 0;
    grid.forEachCandidate({390, 10}, [&](uint32_t) { ++hits; });
    CHECK(hits == 0);
    // A diagonal wall should not occupy the whole bounding box.
    CHECK(grid.cellEntries() < 20u * 15u / 2u);
}
