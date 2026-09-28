#include <algorithm>
#include <random>
#include <set>

#include "physics/particle_grid.hpp"
#include "test.hpp"

using namespace spill;

TEST("grid finds every neighbour a brute force search finds") {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> ux(0.0f, 200.0f), uy(0.0f, 120.0f);
    std::vector<Vec2> pts(600);
    for (auto& p : pts) p = {ux(rng), uy(rng)};

    const float h = 10.0f;
    ParticleGrid grid;
    grid.configure({{0, 0}, {200, 120}}, h);
    grid.build(pts);

    for (size_t i = 0; i < pts.size(); i += 37) {
        std::set<uint32_t> fromGrid;
        grid.forEachNearby(pts[i], [&](uint32_t j) {
            if (lengthSq(pts[j] - pts[i]) < h * h) fromGrid.insert(j);
        });
        std::set<uint32_t> brute;
        for (uint32_t j = 0; j < pts.size(); ++j) {
            if (lengthSq(pts[j] - pts[i]) < h * h) brute.insert(j);
        }
        CHECK(fromGrid == brute);
    }
}

TEST("grid clamps out-of-bounds particles into border cells") {
    std::vector<Vec2> pts{{-50, -50}, {500, 500}, {5, 5}};
    ParticleGrid grid;
    grid.configure({{0, 0}, {100, 100}}, 10.0f);
    grid.build(pts);
    CHECK(grid.countInCell(0, 0) == 2);
    CHECK(grid.countInCell(grid.cols() - 1, grid.rows() - 1) == 1);
}

TEST("grid rebuild handles a shrinking particle set") {
    std::vector<Vec2> pts(50, Vec2{15, 15});
    ParticleGrid grid;
    grid.configure({{0, 0}, {100, 100}}, 10.0f);
    grid.build(pts);
    pts.resize(3);
    grid.build(pts);
    int seen = 0;
    grid.forEachNearby({15, 15}, [&](uint32_t) { ++seen; });
    CHECK(seen == 3);
}
