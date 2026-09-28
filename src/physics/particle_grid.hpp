#pragma once

#include <cstdint>
#include <vector>

#include "physics/geometry.hpp"

namespace spill {

// Uniform grid over a fixed rectangle, rebuilt from scratch every step with a
// counting sort. Particles in the same cell end up contiguous in memory, which
// is what makes neighbour search cheap. Positions outside the bounds are
// clamped into the border cells, so nothing is ever dropped.
class ParticleGrid {
public:
    void configure(Aabb bounds, float cellSize);
    void build(const std::vector<Vec2>& positions);

    // Calls f(j) for every particle in the 3x3 block of cells around p.
    template <class F>
    void forEachNearby(Vec2 p, F&& f) const {
        const int cx = cellX(p.x);
        const int cy = cellY(p.y);
        for (int y = cy - 1; y <= cy + 1; ++y) {
            if (y < 0 || y >= rows_) continue;
            for (int x = cx - 1; x <= cx + 1; ++x) {
                if (x < 0 || x >= cols_) continue;
                const int cell = y * cols_ + x;
                for (uint32_t k = cellStart_[cell]; k < cellStart_[cell + 1]; ++k) {
                    f(sorted_[k]);
                }
            }
        }
    }

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    float cellSize() const { return cellSize_; }
    uint32_t countInCell(int x, int y) const;

private:
    int cellX(float x) const;
    int cellY(float y) const;

    Aabb bounds_{};
    float cellSize_ = 1.0f;
    float invCellSize_ = 1.0f;
    int cols_ = 0;
    int rows_ = 0;
    std::vector<uint32_t> cellOf_;
    std::vector<uint32_t> cellStart_;
    std::vector<uint32_t> sorted_;
    std::vector<uint32_t> cursor_;
};

}  // namespace spill
