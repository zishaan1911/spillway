#include "app/app.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "render/scene.hpp"

namespace spill {

namespace fs = std::filesystem;

namespace {

constexpr int kWidth = 1280;
constexpr int kHeight = 720;
constexpr int kMaxUpdatesPerFrame = 2;

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

// Levels live next to the executable, falling back to the working directory
// so running from the source tree works too.
fs::path findLevelDir() {
    const fs::path candidates[] = {fs::path(GetApplicationDirectory()) / "levels", "levels"};
    for (const fs::path& p : candidates) {
        std::error_code ec;
        if (fs::is_directory(p, ec)) return p;
    }
    return {};
}

}  // namespace

App::App() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(kWidth, kHeight, "Spillway");
    SetWindowMinSize(640, 360);
    SetExitKey(KEY_NULL);  // Escape pauses instead of quitting
    SetTargetFPS(60);

    ui::loadFont();
    frame_ = LoadRenderTexture(kWidth, kHeight);
    SetTextureFilter(frame_.texture, TEXTURE_FILTER_BILINEAR);
    fluid_.load(kWidth, kHeight);

    progressPath_ = (fs::path(GetApplicationDirectory()) / "progress.dat").string();
    progress_ = Progress::load(progressPath_);
    loadLevels();
}

App::~App() {
    fluid_.unload();
    UnloadRenderTexture(frame_);
    ui::unloadFont();
    CloseWindow();
}

void App::loadLevels() {
    const fs::path dir = findLevelDir();
    if (dir.empty()) {
        levelErrors_.push_back("levels folder not found");
        return;
    }
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".lvl") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& f : files) {
        ParseResult r = parseLevel(readFile(f), f.stem().string());
        if (r.level) {
            levels_.push_back(std::move(*r.level));
        } else {
            levelErrors_.push_back(f.filename().string() + ": " + r.error);
        }
    }
}

bool App::unlocked(size_t index) const {
    return index == 0 || progress_.get(levels_[index - 1].id).stars > 0;
}

void App::startLevel(size_t index) {
    current_ = index;
    session_ = std::make_unique<Session>(levels_[index]);
    screen_ = Screen::Play;
    paused_ = false;
    recorded_ = false;
    newBest_ = false;
    accumulator_ = 0.0;
}

void App::request(Action a, size_t level) {
    pending_ = a;
    pendingLevel_ = level;
}

void App::applyPending() {
    const Action a = pending_;
    pending_ = Action::None;
    switch (a) {
        case Action::None: break;
        case Action::Resume: paused_ = false; break;
        case Action::Restart: startLevel(current_); break;
        case Action::Menu: screen_ = Screen::Menu; break;
        case Action::Start: startLevel(pendingLevel_); break;
    }
}

void App::run() {
    while (!WindowShouldClose()) {
        tick();
    }
}

bool App::capture(const CaptureRequest& request) {
    if (request.level >= levels_.size()) return false;
    startLevel(request.level);
    session_->release();
    const int frames = static_cast<int>(request.seconds / Session::kFrameDt);
    for (int i = 0; i < frames; ++i) session_->update();
    time_ = request.seconds;

    BeginDrawing();
    BeginTextureMode(frame_);
    ClearBackground(palette::kBackground);
    drawPlay();
    EndTextureMode();
    EndDrawing();

    Image img = LoadImageFromTexture(frame_.texture);
    ImageFlipVertical(&img);
    const bool ok = ExportImage(img, request.file.c_str());
    UnloadImage(img);
    return ok;
}

void App::tick() {
    viewport_ = ui::Viewport::fit(GetScreenWidth(), GetScreenHeight(), kWidth, kHeight);
    mouse_ = viewport_.toVirtual(GetMousePosition());
    clicked_ = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    time_ += GetFrameTime();

    if (screen_ == Screen::Menu) {
        updateMenu();
    } else {
        updatePlay();
    }

    BeginTextureMode(frame_);
    ClearBackground(palette::kBackground);
    if (screen_ == Screen::Menu) {
        drawMenu();
    } else {
        drawPlay();
    }
    EndTextureMode();

    // Fluid gets its own render target, which must not be nested inside the
    // frame's, so it is rendered first and composited in drawPlay.
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(frame_.texture, {0, 0, static_cast<float>(kWidth), -static_cast<float>(kHeight)},
                   {viewport_.offset.x, viewport_.offset.y, kWidth * viewport_.scale,
                    kHeight * viewport_.scale},
                   {0, 0}, 0.0f, WHITE);
    EndDrawing();

    applyPending();
}

void App::updateMenu() {}

void App::drawMenu() {
    ui::textCentered("SPILLWAY", kWidth * 0.5f, 60, 72, palette::kText);
    if (!levelErrors_.empty()) {
        float y = 160;
        for (const std::string& e : levelErrors_) {
            ui::textCentered(e.c_str(), kWidth * 0.5f, y, 20, palette::kNoDraw);
            y += 26;
        }
    }
    const int cols = 5;
    const float w = 200, h = 110, gap = 24;
    const float x0 = (kWidth - (cols * w + (cols - 1) * gap)) * 0.5f;
    for (size_t i = 0; i < levels_.size(); ++i) {
        const Rectangle r{x0 + (i % cols) * (w + gap), 220 + (i / cols) * (h + gap), w, h};
        const bool open = unlocked(i);
        char label[64];
        std::snprintf(label, sizeof label, "%zu", i + 1);
        if (ui::button(r, "", mouse_, clicked_, open)) request(Action::Start, i);
        ui::text(label, r.x + 14, r.y + 10, 30, open ? palette::kStroke : palette::kMuted);
        ui::text(levels_[i].name.c_str(), r.x + 14, r.y + 46, 20, open ? palette::kText : palette::kMuted);
        ui::stars({r.x + r.width * 0.5f, r.y + h - 20}, 9, progress_.get(levels_[i].id).stars, 3,
                  palette::kStroke);
    }
}

void App::updatePlay() {
    Session& s = *session_;
    const bool decided = s.state() == Session::State::Won || s.state() == Session::State::Lost;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        if (decided) {
            screen_ = Screen::Menu;
            return;
        }
        paused_ = !paused_;
    }
    if (IsKeyPressed(KEY_F1)) debugView_ = !debugView_;
    if (IsKeyPressed(KEY_R)) {
        startLevel(current_);
        return;
    }
    if (paused_) return;

    if (IsKeyPressed(KEY_SPACE)) s.release();
    if (IsKeyPressed(KEY_Z)) s.undo();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) s.beginStroke(mouse_);
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) s.extendStroke(mouse_);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) s.endStroke();
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) s.eraseAt(mouse_);

    // Fixed-rate physics, decoupled from the display refresh.
    accumulator_ += std::min(GetFrameTime(), 0.1f);
    int updates = 0;
    while (accumulator_ >= Session::kFrameDt && updates < kMaxUpdatesPerFrame) {
        s.update();
        accumulator_ -= Session::kFrameDt;
        ++updates;
    }
    if (updates == kMaxUpdatesPerFrame) accumulator_ = 0.0;  // running slow; drop the backlog

    if (decided && !recorded_) finishLevel();
}

void App::finishLevel() {
    recorded_ = true;
    const Session& s = *session_;
    if (s.state() != Session::State::Won) return;
    newBest_ = progress_.record(s.level().id, s.stars(), s.finishTime());
    progress_.save(progressPath_);
}

void App::drawPlay() {
    const Session& s = *session_;
    // The frame's texture mode is active; pause it to render the fluid field.
    EndTextureMode();
    fluid_.render(s.world().fluid);
    BeginTextureMode(frame_);

    drawBackdrop(s, time_);
    if (debugView_) {
        fluid_.drawParticles(s.world().fluid, s.world().fluid.params().restDensity);
    } else {
        fluid_.draw();
    }
    drawForeground(s, time_);
    drawHud();
    if (paused_) drawPause();
    if (s.state() == Session::State::Won || s.state() == Session::State::Lost) drawResult();
}

void App::drawHud() {
    const Session& s = *session_;
    const Level& l = s.level();
    DrawRectangle(0, 0, kWidth, 48, Fade(BLACK, 0.45f));

    char buf[128];
    std::snprintf(buf, sizeof buf, "%zu  %s", current_ + 1, l.name.c_str());
    ui::text(buf, 16, 12, 24, palette::kText);

    // Ink gauge.
    const float inkFrac = l.ink > 0 ? s.inkLeft() / l.ink : 0.0f;
    const Rectangle ink{470, 17, 180, 14};
    DrawRectangleRounded(ink, 1.0f, 6, Fade(palette::kStroke, 0.15f));
    DrawRectangleRounded({ink.x, ink.y, ink.width * inkFrac, ink.height}, 1.0f, 6, palette::kStroke);
    DrawLine(static_cast<int>(ink.x + ink.width * 0.5f), 14, static_cast<int>(ink.x + ink.width * 0.5f),
             34, Fade(palette::kText, 0.5f));
    ui::textRight("INK", ink.x - 10, 14, 20, palette::kMuted);

    // Goal counter.
    std::snprintf(buf, sizeof buf, "%d / %d", std::min(s.inGoal(), l.target), l.target);
    ui::textRight("GOAL", 760, 14, 20, palette::kMuted);
    ui::text(buf, 770, 12, 24, s.inGoal() >= l.target ? palette::kGoal : palette::kText);

    // Clock against par.
    std::snprintf(buf, sizeof buf, "%.1fs", s.elapsed());
    ui::textRight("TIME", 1010, 14, 20, palette::kMuted);
    ui::text(buf, 1020, 12, 24, s.elapsed() <= l.par ? palette::kText : palette::kMuted);
    std::snprintf(buf, sizeof buf, "par %.0fs", l.par);
    ui::textRight(buf, kWidth - 16, 14, 20, palette::kMuted);

    if (s.state() == Session::State::Planning) {
        const char* msg = l.hint.empty() ? "Draw with the left mouse button." : l.hint.c_str();
        ui::textCentered(msg, kWidth * 0.5f, kHeight - 70, 22, palette::kText);
        ui::textCentered("SPACE opens the taps   RMB erase   Z undo   R restart   F1 particles",
                         kWidth * 0.5f, kHeight - 40, 18, palette::kMuted);
    }
}

void App::drawPause() {
    DrawRectangle(0, 0, kWidth, kHeight, Fade(BLACK, 0.55f));
    ui::textCentered("PAUSED", kWidth * 0.5f, 220, 56, palette::kText);
    const float x = kWidth * 0.5f - 120;
    if (ui::button({x, 320, 240, 52}, "Resume", mouse_, clicked_)) request(Action::Resume);
    if (ui::button({x, 386, 240, 52}, "Restart", mouse_, clicked_)) request(Action::Restart);
    if (ui::button({x, 452, 240, 52}, "Levels", mouse_, clicked_)) request(Action::Menu);
}

void App::drawResult() {
    const Session& s = *session_;
    const bool won = s.state() == Session::State::Won;
    const Rectangle panel{kWidth * 0.5f - 260, 180, 520, 330};
    DrawRectangleRounded(panel, 0.08f, 8, Fade(Color{16, 21, 31, 255}, 0.94f));
    DrawRectangleRoundedLinesEx(panel, 0.08f, 8, 2.0f, won ? palette::kGoal : palette::kDrain);

    const float cx = kWidth * 0.5f;
    ui::textCentered(won ? "FILLED" : "SPILLED", cx, panel.y + 26, 48, won ? palette::kGoal : palette::kDrain);
    char buf[128];
    if (won) {
        ui::stars({cx, panel.y + 118}, 22, s.stars(), 3, palette::kStroke);
        std::snprintf(buf, sizeof buf, "%.1fs  (par %.0fs)     ink %.0f%%", s.finishTime(), s.level().par,
                      100.0f * s.inkUsed() / std::max(1.0f, s.level().ink));
        ui::textCentered(buf, cx, panel.y + 160, 22, palette::kText);
        if (newBest_) ui::textCentered("new best", cx, panel.y + 190, 20, palette::kStroke);
    } else {
        ui::textCentered(s.lossReason().c_str(), cx, panel.y + 120, 22, palette::kText);
    }

    const float by = panel.y + panel.height - 76;
    if (ui::button({panel.x + 24, by, 150, 52}, "Levels", mouse_, clicked_)) request(Action::Menu);
    if (ui::button({cx - 75, by, 150, 52}, "Retry", mouse_, clicked_)) request(Action::Restart);
    const bool hasNext = won && current_ + 1 < levels_.size();
    if (ui::button({panel.x + panel.width - 174, by, 150, 52}, "Next", mouse_, clicked_, hasNext)) {
        request(Action::Start, current_ + 1);
    }
}

}  // namespace spill
