#include <cstdio>

extern int test_series();
extern int test_indicator();
extern int test_viewport();
extern int test_frame();

int main() {
    int failed = 0;
    failed += test_series();
    failed += test_indicator();
    failed += test_viewport();
    failed += test_frame();
    if (failed == 0) {
        std::printf("[OK] all tests passed\n");
        return 0;
    }
    std::printf("[FAIL] %d test(s) failed\n", failed);
    return 1;
}
