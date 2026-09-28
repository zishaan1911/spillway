#pragma once

#include "physics/fluid.hpp"
#include "raylib.h"

namespace spill {

// Draws the fluid as a continuous surface instead of a cloud of dots.
//
// Pass 1 splats a soft radial blob per particle into an off-screen field
// texture with additive blending. Alpha accumulates "how much fluid is here"
// and red accumulates speed weighted the same way, so red / alpha is the
// local average speed.
// Pass 2 thresholds that field in a shader: inside the threshold is water,
// shaded deeper where the field is dense, whitened where it is fast (foam),
// with a bright rim along the surface.
class FluidRenderer {
public:
    void load(int width, int height);
    void unload();

    void render(const Fluid& fluid);  // pass 1, call outside any texture mode
    void draw() const;                // pass 2, composites onto the current target

    // Debug view: every particle as a dot coloured by its density.
    void drawParticles(const Fluid& fluid, float restDensity) const;

    Color water{40, 140, 230, 255};
    Color foam{225, 245, 255, 255};

private:
    RenderTexture2D field_{};
    Texture2D blob_{};
    Shader shader_{};
    int locWater_ = -1;
    int locFoam_ = -1;
    int locThreshold_ = -1;
    int width_ = 0;
    int height_ = 0;
    bool loaded_ = false;
};

}  // namespace spill
