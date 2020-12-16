#include "rng.h"

pcg::pcg()
    : m_state(0x853c49e6748fea9bULL)
    , m_inc(0xda3e39cb94b95bdbULL)
{
}

pcg::pcg(std::random_device& rd) { seed(rd); }

void pcg::seed(std::random_device& rd)
{
    uint64_t s0 = uint64_t(rd()) << 31 | uint64_t(rd());
    uint64_t s1 = uint64_t(rd()) << 31 | uint64_t(rd());

    m_state = 0;
    m_inc = (s1 << 1) | 1;
    (void)operator()();
    m_state += s0;
    (void)operator()();
}

pcg::result_type pcg::operator()()
{
    uint64_t oldstate = m_state;
    m_state = oldstate * 6364136223846793005ULL + m_inc;
    uint32_t xorshifted = uint32_t(((oldstate >> 18u) ^ oldstate) >> 27u);
    int rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void pcg::discard(unsigned long long n)
{
    for (unsigned long long i = 0; i < n; ++i)
        operator()();
}

bool operator==(pcg const& lhs, pcg const& rhs) { return lhs.m_state == rhs.m_state && lhs.m_inc == rhs.m_inc; }
bool operator!=(pcg const& lhs, pcg const& rhs) { return lhs.m_state != rhs.m_state || lhs.m_inc != rhs.m_inc; }