#pragma once

#include <cstdint>
#include <vector>

#include "physics/geometry.hpp"

namespace spill {

// Broadphase for static capsules. Each cell lists every capsule whose bounds,
// grown by `margin`, touch the cell, so a point query only has to look at the
// one cell the point falls in. Rebuilt whenever the wall set changes, which is
// rare compared to how often it is queried.
class ColliderGrid {
public:
    void configure(Aabb bounds, float cellSize, float margin);
    void rebuild(const std::vector<Capsule>& capsules);

    template <class F>
    void forEachCandidate(Vec2 p, F&& f) const {
        if (!bounds_.contains(p)) {
            // Rare: stray particles outside the play area test everything.
            for (uint32_t i = 0; i < count_; ++i) f(i);
            return;
        }
        const int cell = cellIndex(p);
        for (uint32_t k = cellStart_[cell]; k < cellStart_[cell + 1]; ++k) f(items_[k]);
    }

    size_t cellEntries() const { return items_.size(); }

private:
    int cellIndex(Vec2 p) const;

    Aabb bounds_{};
    float invCellSize_ = 1.0f;
    float margin_ = 0.0f;
    int cols_ = 0;
    int rows_ = 0;
    uint32_t count_ = 0;
    std::vector<uint32_t> cellStart_;
    std::vector<uint32_t> items_;
};

}  // namespace spill
