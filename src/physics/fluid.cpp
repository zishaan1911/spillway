#include "physics/fluid.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

void Fluid::configure(Aabb bounds, const FluidParams& params) {
    params_ = params;
    grid_.configure(bounds, params.radius);
}

uint32_t Fluid::add(Vec2 position, Vec2 velocity) {
    pos.push_back(position);
    prev.push_back(position);
    vel.push_back(velocity);
    density.push_back(0.0f);
    return static_cast<uint32_t>(pos.size() - 1);
}

void Fluid::clear() {
    pos.clear();
    prev.clear();
    vel.clear();
    density.clear();
    pairs_.clear();
}

void Fluid::predict(float dt) {
    for (size_t i = 0; i < pos.size(); ++i) {
        prev[i] = pos[i];
        pos[i] += vel[i] * dt;
    }
}

void Fluid::findPairs() {
    grid_.build(pos);
    pairs_.clear();
    const float h = params_.radius;
    const float h2 = h * h;
    const float invH = 1.0f / h;
    for (uint32_t i = 0; i < pos.size(); ++i) {
        const Vec2 pi = pos[i];
        grid_.forEachNearby(pi, [&](uint32_t j) {
            if (j <= i) return;
            const Vec2 d = pos[j] - pi;
            const float r2 = lengthSq(d);
            if (r2 >= h2) return;
            if (r2 < 1e-12f) {
                // Coincident particles: pick an arbitrary but deterministic axis
                // so relaxation can separate them.
                const float angle = static_cast<float>((i * 7919u + j * 104729u) % 628u) * 0.01f;
                pairs_.push_back({i, j, 0.0f, {std::cos(angle), std::sin(angle)}});
                return;
            }
            const float r = std::sqrt(r2);
            pairs_.push_back({i, j, r * invH, d / r});
        });
    }
}

void Fluid::relax(float dt) {
    const size_t n = pos.size();
    density.assign(n, 0.0f);
    nearDensity_.assign(n, 0.0f);

    for (const Pair& p : pairs_) {
        const float a = 1.0f - p.q;
        const float a2 = a * a;
        const float a3 = a2 * a;
        density[p.i] += a2;
        density[p.j] += a2;
        nearDensity_[p.i] += a3;
        nearDensity_[p.j] += a3;
    }

    pressure_.resize(n);
    nearPressure_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        pressure_[i] = params_.stiffness * (density[i] - params_.restDensity);
        nearPressure_[i] = params_.nearStiffness * nearDensity_[i];
    }

    // Jacobi-style: every pair uses the pressures computed above, and the
    // displacement is shared equally between the two particles.
    const float dt2 = dt * dt;
    const float maxD = params_.maxDisplacement;
    for (const Pair& p : pairs_) {
        const float a = 1.0f - p.q;
        const float pr = 0.5f * (pressure_[p.i] + pressure_[p.j]);
        const float nearPr = 0.5f * (nearPressure_[p.i] + nearPressure_[p.j]);
        float mag = dt2 * (pr * a + nearPr * a * a);
        mag = std::clamp(mag, -maxD, maxD);
        const Vec2 d = p.n * (0.5f * mag);
        pos[p.i] -= d;
        pos[p.j] += d;
    }
}

void Fluid::deriveVelocities(float dt, float maxSpeed) {
    const float invDt = 1.0f / dt;
    const float max2 = maxSpeed * maxSpeed;
    for (size_t i = 0; i < pos.size(); ++i) {
        Vec2 v = (pos[i] - prev[i]) * invDt;
        const float s2 = lengthSq(v);
        if (s2 > max2) v *= maxSpeed / std::sqrt(s2);
        vel[i] = v;
    }
}

void Fluid::applyViscosity(float dt) {
    const float sigma = params_.viscosityLinear;
    const float beta = params_.viscosityQuadratic;
    for (const Pair& p : pairs_) {
        const float u = dot(vel[p.i] - vel[p.j], p.n);
        if (u <= 0.0f) continue;  // only damp approaching pairs
        // I = dt (1 - q) (sigma u + beta u^2) n, written as a fraction of u and
        // capped so a single pair can never reverse its relative velocity.
        const float fraction = std::min(dt * (1.0f - p.q) * (sigma + beta * u), 1.0f);
        const Vec2 impulse = p.n * (0.5f * u * fraction);
        vel[p.i] -= impulse;
        vel[p.j] += impulse;
    }
}

}  // namespace spill
