#include <unity.h>

extern void run_pid_tests(void);
extern void run_overcurrent_tests(void);
extern void run_main_tests(void);

int main(void) {
    UNITY_BEGIN();

    run_pid_tests();
    run_overcurrent_tests();
    run_main_tests();

    return UNITY_END();
}