#include "physics/particle_grid.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

void ParticleGrid::configure(Aabb bounds, float cellSize) {
    bounds_ = bounds;
    cellSize_ = cellSize;
    invCellSize_ = 1.0f / cellSize;
    const Vec2 size = bounds.size();
    cols_ = std::max(1, static_cast<int>(std::ceil(size.x * invCellSize_)));
    rows_ = std::max(1, static_cast<int>(std::ceil(size.y * invCellSize_)));
    cellStart_.assign(static_cast<size_t>(cols_) * rows_ + 1, 0);
}

int ParticleGrid::cellX(float x) const {
    const int c = static_cast<int>((x - bounds_.min.x) * invCellSize_);
    return std::clamp(c, 0, cols_ - 1);
}

int ParticleGrid::cellY(float y) const {
    const int c = static_cast<int>((y - bounds_.min.y) * invCellSize_);
    return std::clamp(c, 0, rows_ - 1);
}

void ParticleGrid::build(const std::vector<Vec2>& positions) {
    const size_t n = positions.size();
    const size_t cellCount = static_cast<size_t>(cols_) * rows_;
    cellOf_.resize(n);
    sorted_.resize(n);
    std::fill(cellStart_.begin(), cellStart_.end(), 0u);

    // Count particles per cell, shifted by one so the prefix sum below turns
    // counts into start offsets in place.
    for (size_t i = 0; i < n; ++i) {
        const uint32_t cell = static_cast<uint32_t>(cellY(positions[i].y) * cols_ +
                                                    cellX(positions[i].x));
        cellOf_[i] = cell;
        ++cellStart_[cell + 1];
    }
    for (size_t c = 0; c < cellCount; ++c) cellStart_[c + 1] += cellStart_[c];

    // Scatter, with one write cursor per cell.
    cursor_.assign(cellStart_.begin(), cellStart_.end() - 1);
    for (size_t i = 0; i < n; ++i) {
        sorted_[cursor_[cellOf_[i]]++] = static_cast<uint32_t>(i);
    }
}

uint32_t ParticleGrid::countInCell(int x, int y) const {
    const int cell = y * cols_ + x;
    return cellStart_[cell + 1] - cellStart_[cell];
}

}  // namespace spill
