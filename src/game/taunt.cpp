#include "game/taunt.hpp"

#include <array>
#include <cstdio>

namespace spill {

namespace {

template <size_t N>
std::string pick(const std::array<const char*, N>& lines, int spills) {
    return lines[static_cast<size_t>(spills > 0 ? spills - 1 : 0) % N];
}

std::string format(const char* pattern, int a, int b) {
    char buf[160];
    std::snprintf(buf, sizeof buf, pattern, a, b);
    return buf;
}

}  // namespace

std::string taunt(const Session& s, int spills) {
    const int target = s.level().target;
    const int peak = s.peakInGoal();

    if (peak >= target) {
        static constexpr std::array<const char*, 3> kHeld = {
            "It was full. Briefly. That doesn't count.",
            "You had it. Then physics had it.",
            "Full for a moment. The goal wanted longer.",
        };
        return pick(kHeld, spills);
    }
    if (peak >= target * 9 / 10) {
        static constexpr std::array<const char*, 4> kClose = {
            "%d / %d. So close. Not close enough.",
            "%d / %d. You'll be thinking about this one.",
            "%d / %d. A few drops. That's all it was.",
            "%d / %d. Again. Go on.",
        };
        return format(pick(kClose, spills).c_str(), peak, target);
    }

    switch (s.loss()) {
        case Session::Loss::NotEnoughWater: {
            static constexpr std::array<const char*, 4> kLines = {
                "Most of it went on the floor.",
                "The drain says thanks.",
                "That was all the water. All of it.",
                "Gravity: 1. You: 0.",
            };
            return pick(kLines, spills);
        }
        case Session::Loss::Settled: {
            static constexpr std::array<const char*, 3> kLines = {
                "It's not moving and neither is your star count.",
                "Still water. Still losing.",
                "It settled. You shouldn't.",
            };
            return pick(kLines, spills);
        }
        case Session::Loss::NeverArrived: {
            static constexpr std::array<const char*, 3> kLines = {
                "Fifteen seconds of nothing.",
                "The water had other plans.",
                "It went everywhere except there.",
            };
            return pick(kLines, spills);
        }
        case Session::Loss::None:
            break;
    }
    return "Spilled.";
}

std::string winQuip(int spills) {
    if (spills <= 0) return {};
    if (spills == 1) return "Only took one spill.";
    if (spills < 5) return format("Only took %d spills.", spills, 0);
    if (spills < 20) return format("%d spills. Worth it?", spills, 0);
    return format("%d spills. Unhinged. Respect.", spills, 0);
}

}  // namespace spill
