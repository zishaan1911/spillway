#include <cstdio>

#include "test.hpp"

int main() {
    int failedCases = 0;
    for (const auto& c : test::registry()) {
        const int before = test::failures();
        c.fn();
        const bool ok = test::failures() == before;
        std::printf("[%s] %s\n", ok ? " ok " : "FAIL", c.name);
        if (!ok) ++failedCases;
    }
    std::printf("\n%zu tests, %d failed\n", test::registry().size(), failedCases);
    return failedCases == 0 ? 0 : 1;
}
