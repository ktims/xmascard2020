#pragma once

#include <random>
#include <string>

#include "config.h"

#define debug_str(x)                                                                                                   \
    do {                                                                                                               \
        if (DEBUG)                                                                                                     \
            uart_writes(x);                                                                                            \
    } while (0)

#define _BV(x) ((1 << x))

void uart_writes(const std::string&);

void cpu_sleep();
void cpu_off();

uint32_t get_true_random_seed();

inline uint16_t sat_add(uint16_t a, uint16_t b)
{
    uint16_t c = a + b;
    if (c < a)
        c = -1;
    return c;
}

inline uint16_t sat_sub(uint16_t a, uint16_t b)
{
    uint16_t c = a - b;
    if (c > a)
        c = 0;
    return c;
}

inline uint16_t sat_ofs(uint16_t a, int16_t ofs)
{
    if (ofs >= 0)
        return sat_add(a, ofs);
    else
        return sat_sub(a, -ofs);
}

uint32_t get_true_random();