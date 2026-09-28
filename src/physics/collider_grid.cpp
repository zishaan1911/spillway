#include "physics/collider_grid.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

void ColliderGrid::configure(Aabb bounds, float cellSize, float margin) {
    bounds_ = bounds;
    invCellSize_ = 1.0f / cellSize;
    margin_ = margin;
    cols_ = std::max(1, static_cast<int>(std::ceil(bounds.size().x * invCellSize_)));
    rows_ = std::max(1, static_cast<int>(std::ceil(bounds.size().y * invCellSize_)));
    cellStart_.assign(static_cast<size_t>(cols_) * rows_ + 1, 0);
    items_.clear();
    count_ = 0;
}

int ColliderGrid::cellIndex(Vec2 p) const {
    const int x = std::clamp(static_cast<int>((p.x - bounds_.min.x) * invCellSize_), 0, cols_ - 1);
    const int y = std::clamp(static_cast<int>((p.y - bounds_.min.y) * invCellSize_), 0, rows_ - 1);
    return y * cols_ + x;
}

void ColliderGrid::rebuild(const std::vector<Capsule>& capsules) {
    count_ = static_cast<uint32_t>(capsules.size());
    const size_t cellCount = static_cast<size_t>(cols_) * rows_;
    std::vector<std::vector<uint32_t>> perCell(cellCount);

    for (uint32_t i = 0; i < count_; ++i) {
        const Capsule& c = capsules[i];
        const Aabb box = c.bounds().expanded(margin_);
        const int x0 = std::clamp(static_cast<int>((box.min.x - bounds_.min.x) * invCellSize_), 0, cols_ - 1);
        const int x1 = std::clamp(static_cast<int>((box.max.x - bounds_.min.x) * invCellSize_), 0, cols_ - 1);
        const int y0 = std::clamp(static_cast<int>((box.min.y - bounds_.min.y) * invCellSize_), 0, rows_ - 1);
        const int y1 = std::clamp(static_cast<int>((box.max.y - bounds_.min.y) * invCellSize_), 0, rows_ - 1);
        const float cellSize = 1.0f / invCellSize_;
        const float reach = c.radius + margin_ + cellSize * 0.7072f;  // half cell diagonal
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                // Skip cells the box touches but the capsule itself doesn't,
                // which matters for long diagonal walls.
                const Vec2 centre = bounds_.min + Vec2{(x + 0.5f) * cellSize, (y + 0.5f) * cellSize};
                if (distanceToSegment(centre, c.a, c.b) > reach) continue;
                perCell[static_cast<size_t>(y) * cols_ + x].push_back(i);
            }
        }
    }

    items_.clear();
    for (size_t cell = 0; cell < cellCount; ++cell) {
        cellStart_[cell] = static_cast<uint32_t>(items_.size());
        items_.insert(items_.end(), perCell[cell].begin(), perCell[cell].end());
    }
    cellStart_[cellCount] = static_cast<uint32_t>(items_.size());
}

}  // namespace spill
