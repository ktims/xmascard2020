#pragma once

#include <random>
#include <functional>

// The C++ included RNGs suck and are large
// PCG code refactored from https://arvid.io/2018/07/02/better-cxx-prng/
// Which is almost verbatim from https://github.com/imneme/pcg-c-basic
class pcg {
public:
    using result_type = uint32_t;
    static constexpr result_type(min)() { return 0; }
    static constexpr result_type(max)() { return UINT32_MAX; }
    friend bool operator==(pcg const&, pcg const&);
    friend bool operator!=(pcg const&, pcg const&);

    pcg();
    explicit pcg(std::function<uint32_t(void)> rd);

    void seed(std::function<uint32_t(void)> rd);

    result_type operator()();

    void discard(unsigned long long n);

private:
    uint64_t m_state;
    uint64_t m_inc;
};

bool operator==(pcg const& lhs, pcg const& rhs);
bool operator!=(pcg const& lhs, pcg const& rhs);
