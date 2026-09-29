#pragma once

#include <string>

#include "game/session.hpp"

namespace spill {

// What the results screen says after a lost level. `spills` is how many times
// this level has now been lost, which picks the line, so retrying cycles
// through them instead of repeating the same one.
//
// Near misses name the margin, because that is what stings.
std::string taunt(const Session& session, int spills);

// A line for a win, if the player had to earn it. Empty on a clean first try.
std::string winQuip(int spills);

}  // namespace spill
