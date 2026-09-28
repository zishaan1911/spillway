#include "render/fluid_renderer.hpp"

#include <algorithm>
#include <cmath>

#include "rlgl.h"

namespace spill {

namespace {

constexpr int kBlobSize = 64;
constexpr float kBlobRadius = 13.0f;   // world units covered by one splat
constexpr float kSplatStrength = 0.5f;  // keeps sums below 8-bit saturation
constexpr float kFoamSpeed = 700.0f;    // speed that renders as pure foam
constexpr float kThreshold = 0.34f;

const char* kFragment = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 waterColor;
uniform vec4 foamColor;
uniform float threshold;
out vec4 finalColor;

void main() {
    vec4 s = texture(texture0, fragTexCoord);
    float field = s.a;
    if (field < threshold) discard;

    float speed = clamp(s.r / max(field, 1e-3), 0.0, 1.0);
    float depth = smoothstep(threshold, 1.0, field);
    vec3 col = mix(waterColor.rgb * 1.35, waterColor.rgb * 0.8, depth);
    col = mix(col, foamColor.rgb, speed * speed);

    float rim = 1.0 - smoothstep(threshold, threshold + 0.07, field);
    col = mix(col, foamColor.rgb, rim * 0.75);
    finalColor = vec4(col, mix(0.82, 0.95, depth));
}
)";

// Premultiplied soft disc: every channel carries the same falloff.
Texture2D makeBlob() {
    Image img = GenImageColor(kBlobSize, kBlobSize, BLANK);
    auto* px = static_cast<Color*>(img.data);
    const float c = (kBlobSize - 1) * 0.5f;
    for (int y = 0; y < kBlobSize; ++y) {
        for (int x = 0; x < kBlobSize; ++x) {
            const float dx = (x - c) / c;
            const float dy = (y - c) / c;
            const float r2 = dx * dx + dy * dy;
            const float f = r2 >= 1.0f ? 0.0f : (1.0f - r2) * (1.0f - r2);
            const auto v = static_cast<unsigned char>(std::lround(f * 255.0f));
            px[y * kBlobSize + x] = {v, v, v, v};
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

void setColor(const Shader& s, int loc, Color c) {
    const float v[4] = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
    SetShaderValue(s, loc, v, SHADER_UNIFORM_VEC4);
}

}  // namespace

void FluidRenderer::load(int width, int height) {
    width_ = width;
    height_ = height;
    field_ = LoadRenderTexture(width, height);
    SetTextureFilter(field_.texture, TEXTURE_FILTER_BILINEAR);
    blob_ = makeBlob();
    shader_ = LoadShaderFromMemory(nullptr, kFragment);
    locWater_ = GetShaderLocation(shader_, "waterColor");
    locFoam_ = GetShaderLocation(shader_, "foamColor");
    locThreshold_ = GetShaderLocation(shader_, "threshold");
    loaded_ = true;
}

void FluidRenderer::unload() {
    if (!loaded_) return;
    UnloadShader(shader_);
    UnloadTexture(blob_);
    UnloadRenderTexture(field_);
    loaded_ = false;
}

void FluidRenderer::render(const Fluid& fluid) {
    BeginTextureMode(field_);
    ClearBackground(BLANK);
    BeginBlendMode(BLEND_ADD_COLORS);  // plain sum: dst = src + dst
    const Rectangle src{0, 0, static_cast<float>(kBlobSize), static_cast<float>(kBlobSize)};
    const float size = kBlobRadius * 2.0f;
    const auto strength = static_cast<unsigned char>(kSplatStrength * 255.0f);
    for (size_t i = 0; i < fluid.size(); ++i) {
        const Vec2 p = fluid.pos[i];
        const float s = std::min(length(fluid.vel[i]) / kFoamSpeed, 1.0f);
        const auto red = static_cast<unsigned char>(s * strength);
        DrawTexturePro(blob_, src, {p.x - kBlobRadius, p.y - kBlobRadius, size, size}, {0, 0}, 0.0f,
                       {red, 0, strength, strength});
    }
    EndBlendMode();
    EndTextureMode();
}

void FluidRenderer::draw() const {
    BeginShaderMode(shader_);
    setColor(shader_, locWater_, water);
    setColor(shader_, locFoam_, foam);
    SetShaderValue(shader_, locThreshold_, &kThreshold, SHADER_UNIFORM_FLOAT);
    // Render textures are stored upside down.
    DrawTextureRec(field_.texture,
                   {0, 0, static_cast<float>(width_), -static_cast<float>(height_)}, {0, 0},
                   WHITE);
    EndShaderMode();
}

void FluidRenderer::drawParticles(const Fluid& fluid, float restDensity) const {
    for (size_t i = 0; i < fluid.size(); ++i) {
        // Blue at rest density, red when compressed, green when sparse.
        const float d = fluid.density[i] / restDensity;
        const Color c = d > 1.0f ? ColorLerp(SKYBLUE, RED, std::min(d - 1.0f, 1.0f))
                                 : ColorLerp(LIME, SKYBLUE, d);
        DrawCircleV({fluid.pos[i].x, fluid.pos[i].y}, 2.5f, c);
    }
}

}  // namespace spill
