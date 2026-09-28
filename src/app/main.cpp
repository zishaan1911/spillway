#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "app/app.hpp"

// spillway                               play
// spillway --capture LEVEL SECONDS FILE [solved]
//                                        render a level (or "menu") to a PNG
//                                        and exit
int main(int argc, char** argv) {
    spill::App app;
    if ((argc == 5 || argc == 6) && std::strcmp(argv[1], "--capture") == 0) {
        spill::CaptureRequest req;
        req.menu = std::strcmp(argv[2], "menu") == 0;
        req.level = static_cast<size_t>(std::max(1, std::atoi(argv[2])) - 1);
        req.seconds = static_cast<float>(std::atof(argv[3]));
        req.file = argv[4];
        req.solved = argc == 6 && std::strcmp(argv[5], "solved") == 0;
        return app.capture(req) ? 0 : 1;
    }
    app.run();
    return 0;
}
