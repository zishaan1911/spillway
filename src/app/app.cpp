#include "app/app.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "game/taunt.hpp"
#include "render/scene.hpp"
#include "rlgl.h"

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
    SetTraceLogLevel(LOG_WARNING);
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
    abandonAttempt();
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
        case Action::Menu:
            abandonAttempt();
            screen_ = Screen::Menu;
            break;
        case Action::Start: startLevel(pendingLevel_); break;
    }
}

void App::run() {
    while (!WindowShouldClose() && !quit_) {
        tick();
    }
}

bool App::capture(const CaptureRequest& request) {
    if (request.menu) {
        screen_ = Screen::Menu;
    } else {
        if (request.level >= levels_.size()) return false;
        startLevel(request.level);
        if (request.solved) {
            for (const auto& stroke : session_->level().solution) session_->drawPolyline(stroke);
        }
        session_->release();
    }
    mouse_ = {-100.0f, -100.0f};  // no hover highlights

    const int frames = static_cast<int>(request.seconds / Session::kFrameDt);
    int written = 0;
    for (int i = 0; i < frames; ++i) {
        if (request.menu) {
            updateDemo();
        } else {
            session_->update();
        }
        time_ = (i + 1) * Session::kFrameDt;
        if (request.every > 0 && i % request.every == 0) {
            char name[32];
            std::snprintf(name, sizeof name, "frame_%04d.png", written++);
            if (!exportFrame((fs::path(request.file) / name).string())) return false;
        }
    }
    return request.every > 0 || exportFrame(request.file);
}

bool App::exportFrame(const std::string& file) {
    BeginDrawing();
    BeginTextureMode(frame_);
    ClearBackground(palette::kBackground);
    if (screen_ == Screen::Menu) {
        drawMenu();
    } else {
        drawPlay();
    }
    EndTextureMode();
    EndDrawing();

    Image img = LoadImageFromTexture(frame_.texture);
    ImageFlipVertical(&img);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);  // see the note in tick()
    const bool ok = ExportImage(img, file.c_str());
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
    // Blending inside the frame leaves its alpha below 1 in places; the colour
    // is already final, so copy it without blending.
    rlDrawRenderBatchActive();
    rlDisableColorBlend();
    DrawTexturePro(frame_.texture, {0, 0, static_cast<float>(kWidth), -static_cast<float>(kHeight)},
                   {viewport_.offset.x, viewport_.offset.y, kWidth * viewport_.scale,
                    kHeight * viewport_.scale},
                   {0, 0}, 0.0f, WHITE);
    rlDrawRenderBatchActive();
    rlEnableColorBlend();
    EndDrawing();

    applyPending();
}

void App::updateMenu() {
    if (IsKeyPressed(KEY_ESCAPE)) quit_ = true;
    updateDemo();
}

// The first level, solved, loops behind the title.
void App::updateDemo() {
    if (levels_.empty()) return;
    if (!demo_) {
        demo_ = std::make_unique<Session>(levels_[0]);
        for (const auto& stroke : levels_[0].solution) demo_->drawPolyline(stroke);
        demo_->release();
        demoRestart_ = 0.0f;
    }
    demo_->update();
    if (demo_->state() == Session::State::Won || demo_->state() == Session::State::Lost) {
        demoRestart_ += Session::kFrameDt;
        if (demoRestart_ > 3.0f) demo_.reset();
    }
}

void App::drawSession(const Session& s) {
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
}

void App::drawMenu() {
    if (demo_) {
        drawSession(*demo_);
        DrawRectangle(0, 0, kWidth, kHeight, Fade(palette::kBackground, 0.72f));
    }
    ui::textCentered("SPILLWAY", kWidth * 0.5f, 56, 84, palette::kText);
    ui::textCentered("a fluid puzzle", kWidth * 0.5f, 146, 24, palette::kMuted);

    int total = 0;
    for (const Level& l : levels_) total += progress_.get(l.id).stars;
    char starText[32];
    std::snprintf(starText, sizeof starText, "%d / %zu", total, levels_.size() * 3);
    const float starW = ui::measure(starText, 24).x + 30;
    ui::star({kWidth * 0.5f - starW * 0.5f + 10, 196}, 11, true, palette::kStroke);
    ui::text(starText, kWidth * 0.5f - starW * 0.5f + 28, 184, 24, palette::kText);
    if (progress_.totalSpills() > 0) {
        std::snprintf(starText, sizeof starText, "%d spills", progress_.totalSpills());
        ui::textCentered(starText, kWidth * 0.5f, 214, 20, palette::kDrain);
    }
    const int cols = 5;
    const float w = 200, h = 110, gap = 24;
    const float x0 = (kWidth - (cols * w + (cols - 1) * gap)) * 0.5f;
    for (size_t i = 0; i < levels_.size(); ++i) {
        const Rectangle r{x0 + (i % cols) * (w + gap), 250 + (i / cols) * (h + gap), w, h};
        const bool open = unlocked(i);
        char label[64];
        std::snprintf(label, sizeof label, "%zu", i + 1);
        if (ui::button(r, "", mouse_, clicked_, open)) request(Action::Start, i);
        ui::text(label, r.x + 14, r.y + 10, 30, open ? palette::kStroke : palette::kMuted);
        ui::text(levels_[i].name.c_str(), r.x + 14, r.y + 46, 20, open ? palette::kText : palette::kMuted);
        const LevelRecord rec = progress_.get(levels_[i].id);
        ui::stars({r.x + r.width * 0.5f, r.y + h - 20}, 9, rec.stars, 3, palette::kStroke);
        if (rec.spills > 0) {
            std::snprintf(label, sizeof label, "%d", rec.spills);
            ui::textRight(label, r.x + r.width - 14, r.y + 14, 20, palette::kDrain);
        }
    }

    float y = 540;
    for (const std::string& e : levelErrors_) {
        ui::textCentered(e.c_str(), kWidth * 0.5f, y, 20, palette::kNoDraw);
        y += 26;
    }
    ui::textCentered("Draw walls to get the water into the goal. Finish a level to unlock the next.",
                     kWidth * 0.5f, kHeight - 92, 20, palette::kMuted);
    if (ui::button({kWidth * 0.5f - 80, kHeight - 60, 160, 44}, "Quit", mouse_, clicked_)) quit_ = true;
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

    if (decided && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
        const bool next = s.state() == Session::State::Won && current_ + 1 < levels_.size();
        request(next ? Action::Start : Action::Restart, current_ + 1);
        return;
    }
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
        const double t0 = GetTime();
        s.update();
        stepMs_ = stepMs_ * 0.9 + (GetTime() - t0) * 1000.0 * 0.1;
        accumulator_ -= Session::kFrameDt;
        ++updates;
    }
    if (updates == kMaxUpdatesPerFrame) accumulator_ = 0.0;  // running slow; drop the backlog

    if (decided && !recorded_) finishLevel();
}

void App::finishLevel() {
    recorded_ = true;
    const Session& s = *session_;
    if (s.state() == Session::State::Won) {
        newBest_ = progress_.record(s.level().id, s.stars(), s.finishTime());
    } else {
        progress_.addSpill(s.level().id);
    }
    progress_.save(progressPath_);
}

// Walking away once the water is running counts as a spill too.
void App::abandonAttempt() {
    if (!session_ || session_->state() != Session::State::Flowing) return;
    progress_.addSpill(session_->level().id);
    progress_.save(progressPath_);
    session_.reset();
}

void App::drawPlay() {
    const Session& s = *session_;
    drawSession(s);
    drawHud();
    if (!paused_ && (s.state() == Session::State::Planning || s.state() == Session::State::Flowing)) {
        // Cursor ring: amber where a stroke can go, red where it cannot.
        const bool ok = s.canDrawAt(mouse_) && s.inkLeft() > 0.0f;
        DrawCircleLinesV(toRl(mouse_), Session::kStrokeRadius + 3.0f,
                         ok ? palette::kStroke : palette::kNoDraw);
    }
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

    if (debugView_) {
        const World& w = s.world();
        std::snprintf(buf, sizeof buf,
                      "particles %zu   pairs %zu   bodies %zu   walls %zu   step %.2f ms   %d fps",
                      w.fluid.size(), w.fluid.pairs().size(), w.bodies.items.size(), w.walls().size(),
                      stepMs_, GetFPS());
        ui::text(buf, 16, 58, 18, palette::kMuted);
    }

    if (s.state() == Session::State::Planning) {
        const char* msg = l.hint.empty() ? "Draw with the left mouse button." : l.hint.c_str();
        ui::textCentered(msg, kWidth * 0.5f, kHeight - 70, 22, palette::kText);
        ui::textCentered("SPACE opens the taps (no erasing after that)   RMB erase   Z undo   R restart",
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
        const std::string quip = winQuip(progress_.get(s.level().id).spills);
        if (!quip.empty()) ui::textCentered(quip.c_str(), cx, panel.y + 192, 20, palette::kMuted);
        if (newBest_) ui::textCentered("new best", cx, panel.y + 218, 20, palette::kStroke);
    } else {
        const int spills = progress_.get(s.level().id).spills;
        ui::textCentered(taunt(s, spills).c_str(), cx, panel.y + 112, 24, palette::kText);
        std::snprintf(buf, sizeof buf, "spill #%d on this level", spills);
        ui::textCentered(buf, cx, panel.y + 156, 20, palette::kMuted);
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
