#include "game/taunt.hpp"
#include "test.hpp"

using namespace spill;

namespace {

// Pours 150 particles at a cup whose goal region stops short of the top, so
// all 150 can never be counted at once.
Session::State pour(Session& s) {
    s.release();
    for (int i = 0; i < 60 * 30 && s.state() == Session::State::Flowing; ++i) s.update();
    return s.state();
}

Level cup(int target) {
    return *parseLevel(
                "ink 300\n"
                "target " + std::to_string(target) + "\n"
                "emitter 640 100  0 1  200 150\n"
                "chain 6  600 500  600 640  680 640  680 500\n"
                "goal 606 560 68 74\n",  // the top of the cup does not count
                "cup")
                .level;
}

}  // namespace

TEST("a near miss names the margin") {
    Session s(cup(150));
    CHECK(pour(s) == Session::State::Lost);
    CHECK(s.peakInGoal() >= 135 && s.peakInGoal() < 150);
    CHECK(taunt(s, 1).find("/ 150") != std::string::npos);
}

TEST("retrying cycles through the lines") {
    Session s(cup(150));
    pour(s);
    CHECK(taunt(s, 1) != taunt(s, 2));
}

TEST("a clean first win needs no comment") {
    CHECK(winQuip(0).empty());
    CHECK(winQuip(1) == "Only took one spill.");
    CHECK(winQuip(3).find("3 spills") != std::string::npos);
    CHECK(winQuip(40).find("40") != std::string::npos);
}
