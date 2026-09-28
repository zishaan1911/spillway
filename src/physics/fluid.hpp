#pragma once

#include <cstdint>
#include <vector>

#include "physics/particle_grid.hpp"

namespace spill {

// Tuning for the particle fluid. Lengths are in world units (one unit is one
// pixel at the game's reference resolution) and time is in seconds.
struct FluidParams {
    float radius = 16.0f;            // interaction radius h
    float restDensity = 3.2f;        // target of sum (1 - r/h)^2
    float stiffness = 18000.0f;      // pressure response to over-density
    float nearStiffness = 36000.0f;  // short-range repulsion, keeps particles apart
    float viscosityLinear = 0.5f;    // sigma
    float viscosityQuadratic = 0.004f;  // beta
    float maxDisplacement = 4.0f;    // per pair per step, a guard against blow-ups
};

// Particle-based viscoelastic fluid after Clavet, Beaudoin and Poulin (2005),
// "Particle-based Viscoelastic Fluid Simulation", without the elastic springs.
// Incompressibility comes from double density relaxation: each particle has a
// density and a near-density, and pairs are pushed apart along their axis in
// proportion to the resulting pressures. It is a position-based scheme, so
// velocities are recovered from how far particles actually moved.
//
// Storage is structure-of-arrays so the hot loops stay tight.
class Fluid {
public:
    struct Pair {
        uint32_t i;
        uint32_t j;
        float q;  // r / h, in [0, 1)
        Vec2 n;   // unit vector from i to j
    };

    void configure(Aabb bounds, const FluidParams& params);
    const FluidParams& params() const { return params_; }

    uint32_t add(Vec2 position, Vec2 velocity);
    void clear();
    size_t size() const { return pos.size(); }

    // Removes every particle for which keep(i) is false. Order is not preserved.
    template <class Keep>
    size_t compact(Keep&& keep) {
        size_t removed = 0;
        size_t i = 0;
        while (i < pos.size()) {
            if (keep(i)) {
                ++i;
                continue;
            }
            const size_t last = pos.size() - 1;
            pos[i] = pos[last];
            prev[i] = prev[last];
            vel[i] = vel[last];
            density[i] = density[last];
            pos.pop_back();
            prev.pop_back();
            vel.pop_back();
            density.pop_back();
            ++removed;
        }
        return removed;
    }

    // The stages of one step, in the order World calls them:
    //   accelerate -> predict -> findPairs -> relax -> (collisions) ->
    //   deriveVelocities -> applyViscosity
    void predict(float dt);
    void findPairs();
    void relax(float dt);
    void deriveVelocities(float dt, float maxSpeed);
    void applyViscosity(float dt);

    const std::vector<Pair>& pairs() const { return pairs_; }

    std::vector<Vec2> pos;
    std::vector<Vec2> prev;
    std::vector<Vec2> vel;
    std::vector<float> density;  // last computed density, handy for rendering

private:
    FluidParams params_{};
    ParticleGrid grid_;
    std::vector<Pair> pairs_;
    std::vector<float> nearDensity_;
    std::vector<float> pressure_;
    std::vector<float> nearPressure_;
};

}  // namespace spill
