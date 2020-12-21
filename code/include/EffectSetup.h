#pragma once

#include "Effects.h"
#include "config.h"

using mbi_t = MBI5043<11, MBI_GCLK_TIMER, MBI_GCLK_TIMER_RCC>;
using effect_fb_t = mbi_t::fb_t&;
using effect_ref = std::reference_wrapper<MBIEffect<mbi_t>>;

// Instantiate all the effects we might want. The compiler should strip any that aren't actually added to the effects
// array / otherwise referenced

auto AllOff = AllToValue<mbi_t, mbi_t::LED_MIN>();
auto AllOn = AllToValue<mbi_t, mbi_t::LED_MAX>();

auto AllRandom = AllRandomRange<mbi_t>();

// This ugly instantiation tho :/. Can't do type inference because mbi_t must be
// defined and that only works if there are no manual template parameters
auto ChaseAround = Chase<mbi_t, decltype(LED_ORDER)::const_iterator>(LED_ORDER.begin(), LED_ORDER.end(), 120, 3);
auto ChaseBack = Chase<mbi_t, decltype(LED_ORDER)::const_iterator>(
    LED_ORDER.begin(), LED_ORDER.end(), 15, mbi_t::N_LEDS, Chase<mbi_t, decltype(LED_ORDER)::const_iterator>::DOWN);

auto FastChase = Chase<mbi_t, decltype(LED_ORDER)::const_iterator>(
    LED_ORDER.begin(), LED_ORDER.end(), 5, 1, Chase<mbi_t, decltype(LED_ORDER)::const_iterator>::DOWN);

auto ChaseRandom = RandomChase<mbi_t>(20, mbi_t::N_LEDS);
auto RandomSequence = RandomChase<mbi_t>(5, 1, 0);

auto RampAllUp = RampAll<mbi_t>(mbi_t::LED_MAX / 128);

auto TwinkleTwinkle = Twinkle<mbi_t>(8192, 40);
auto TwinkleBlinkle = Twinkle<mbi_t>(INT16_MAX / 2, 20);

auto indicator = FirstN<mbi_t>(1, true);