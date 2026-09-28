#include "game/progress.hpp"
#include "test.hpp"

using namespace spill;

TEST("progress keeps the best stars and best time separately") {
    Progress p;
    CHECK(p.record("a", 2, 20.0f));
    CHECK(p.record("a", 1, 15.0f));   // faster, fewer stars
    CHECK(!p.record("a", 1, 30.0f));  // worse on both counts
    CHECK(p.get("a").stars == 2);
    CHECK_NEAR(p.get("a").bestTime, 15.0, 1e-6);
}

TEST("a loss never sets a best time") {
    Progress p;
    CHECK(!p.record("a", 0, 5.0f));
    CHECK(p.get("a").bestTime == 0.0f);
}

TEST("progress round-trips through text") {
    Progress p;
    p.record("01_first_drop", 3, 7.25f);
    p.record("02_ramp", 1, 40.0f);
    const Progress q = Progress::parse(p.serialize());
    CHECK(q.get("01_first_drop").stars == 3);
    CHECK_NEAR(q.get("02_ramp").bestTime, 40.0, 1e-4);
    CHECK(q.totalStars() == 4);
}

TEST("corrupt progress lines are ignored") {
    const Progress p = Progress::parse("good 2 10\nbroken line\nhacked 99 1\n");
    CHECK(p.get("good").stars == 2);
    CHECK(p.get("hacked").stars == 3);
    CHECK(p.get("broken").stars == 0);
}
