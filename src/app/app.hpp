#pragma once

#include <memory>
#include <string>
#include <vector>

#include "app/ui.hpp"
#include "game/progress.hpp"
#include "game/session.hpp"
#include "render/fluid_renderer.hpp"

namespace spill {

// Renders one level to a PNG without opening an interactive session. Used to
// produce the screenshots in the README.
struct CaptureRequest {
    size_t level = 0;
    float seconds = 4.0f;
    std::string file = "capture.png";
    bool solved = false;  // draw the level's reference solution first
};

class App {
public:
    App();
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();
    bool capture(const CaptureRequest& request);

private:
    enum class Screen { Menu, Play };

    // Buttons are drawn and clicked mid-frame; what they do is deferred to the
    // end of the frame so nothing is torn down while it is being drawn.
    enum class Action { None, Resume, Restart, Menu, Start };
    void request(Action a, size_t level = 0);
    void applyPending();

    void loadLevels();
    void startLevel(size_t index);
    bool unlocked(size_t index) const;

    void tick();
    void updateMenu();
    void updatePlay();
    void finishLevel();

    void drawMenu();
    void drawPlay();
    void drawHud();
    void drawPause();
    void drawResult();

    Screen screen_ = Screen::Menu;
    Action pending_ = Action::None;
    size_t pendingLevel_ = 0;
    std::vector<Level> levels_;
    std::vector<std::string> levelErrors_;
    Progress progress_;
    std::string progressPath_;

    std::unique_ptr<Session> session_;
    size_t current_ = 0;
    bool paused_ = false;
    bool debugView_ = false;
    bool recorded_ = false;
    bool newBest_ = false;
    double accumulator_ = 0.0;
    float time_ = 0.0f;

    ui::Viewport viewport_;
    Vec2 mouse_;
    bool clicked_ = false;
    RenderTexture2D frame_{};
    FluidRenderer fluid_;
};

}  // namespace spill
