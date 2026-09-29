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
    bool menu = false;  // the title screen instead of a level
    size_t level = 0;
    float seconds = 4.0f;
    std::string file = "capture.png";
    bool solved = false;  // draw the level's reference solution first
    int every = 0;        // if > 0, `file` is a folder and a frame is saved
                          // every `every` physics frames
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
    void abandonAttempt();

    void updateDemo();
    void drawSession(const Session& s);
    void drawMenu();
    void drawPlay();
    void drawHud();
    void drawPause();
    void drawResult();
    bool exportFrame(const std::string& file);

    Screen screen_ = Screen::Menu;
    Action pending_ = Action::None;
    size_t pendingLevel_ = 0;
    std::vector<Level> levels_;
    std::vector<std::string> levelErrors_;
    Progress progress_;
    std::string progressPath_;

    std::unique_ptr<Session> session_;
    std::unique_ptr<Session> demo_;  // plays behind the title screen
    float demoRestart_ = 0.0f;
    bool quit_ = false;
    size_t current_ = 0;
    bool paused_ = false;
    bool debugView_ = false;
    bool recorded_ = false;
    bool newBest_ = false;
    double accumulator_ = 0.0;
    double stepMs_ = 0.0;  // smoothed cost of one physics frame
    float time_ = 0.0f;

    ui::Viewport viewport_;
    Vec2 mouse_;
    bool clicked_ = false;
    RenderTexture2D frame_{};
    FluidRenderer fluid_;
};

}  // namespace spill
