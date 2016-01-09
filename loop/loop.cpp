void foo() {
    static const constexpr int N = 100;
    for (int i = 0; i < N; ++i) {
        __asm__("");
    }
}
