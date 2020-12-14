#pragma once

#include "Effects.h"
#include "config.h"

using mbi_t = MBI5043<11, MBI_GCLK_TIMER, MBI_GCLK_TIMER_RCC>;
using effect_fb_t = mbi_t::fb_t&;
using effect_ref = std::reference_wrapper<MBIEffect<mbi_t>>;

auto AllOff = AllToValue<mbi_t, mbi_t::LED_MIN>();
auto AllOn = AllToValue<mbi_t, mbi_t::LED_MAX>();

auto AllRandom = AllRandomRange<mbi_t>();

// This ugly instantiation tho :/. Can't do type inference because mbi_t must be
// defined and that only works if there are no manual template parameters
auto ChaseAround = Chase<mbi_t, decltype(LED_ORDER)::iterator>(
    LED_ORDER.begin(), LED_ORDER.end());