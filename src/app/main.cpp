#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "app/app.hpp"

// spillway                                   play
// spillway --capture LEVEL SECONDS FILE [solved]
//                                            render a level (or "menu") to a
//                                            PNG and exit
// spillway --record LEVEL SECONDS DIR [solved]
//                                            save 20 frames a second to DIR
int main(int argc, char** argv) {
    spill::App app;
    const bool capture = argc > 1 && std::strcmp(argv[1], "--capture") == 0;
    const bool record = argc > 1 && std::strcmp(argv[1], "--record") == 0;
    if ((capture || record) && (argc == 5 || argc == 6)) {
        spill::CaptureRequest req;
        req.menu = std::strcmp(argv[2], "menu") == 0;
        req.level = static_cast<size_t>(std::max(1, std::atoi(argv[2])) - 1);
        req.seconds = static_cast<float>(std::atof(argv[3]));
        req.file = argv[4];
        req.solved = argc == 6 && std::strcmp(argv[5], "solved") == 0;
        if (record) {
            req.every = 3;
            std::filesystem::create_directories(req.file);
        }
        return app.capture(req) ? 0 : 1;
    }
    app.run();
    return 0;
}
