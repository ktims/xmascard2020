#pragma once

#include <libopencm3/stm32/timer.h>
#include "config.h"

#ifdef OPTIMIZATIONS

// Stolen directly from libopencm3, but use template variables instead of
// function variables. We only call this once, with one set of parameters, so
// much less code will be built since the compiler can strip code that never
// matches a constexpr.

#define timer_set_oc_mode(timer_peripheral, oc_id, oc_mode) timer_set_oc_mode_cpp<timer_peripheral, oc_id, oc_mode>()
template <uint32_t timer_peripheral, enum tim_oc_id oc_id,
    enum tim_oc_mode oc_mode>
void timer_set_oc_mode_cpp()
{
    switch (oc_id) {
    case TIM_OC1:
        TIM_CCMR1(timer_peripheral) &= ~TIM_CCMR1_CC1S_MASK;
        TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_CC1S_OUT;
        TIM_CCMR1(timer_peripheral) &= ~TIM_CCMR1_OC1M_MASK;
        switch (oc_mode) {
        case TIM_OCM_FROZEN:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_FROZEN;
            break;
        case TIM_OCM_ACTIVE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_ACTIVE;
            break;
        case TIM_OCM_INACTIVE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_INACTIVE;
            break;
        case TIM_OCM_TOGGLE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_TOGGLE;
            break;
        case TIM_OCM_FORCE_LOW:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_FORCE_LOW;
            break;
        case TIM_OCM_FORCE_HIGH:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_FORCE_HIGH;
            break;
        case TIM_OCM_PWM1:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_PWM1;
            break;
        case TIM_OCM_PWM2:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC1M_PWM2;
            break;
        }
        break;
    case TIM_OC2:
        TIM_CCMR1(timer_peripheral) &= ~TIM_CCMR1_CC2S_MASK;
        TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_CC2S_OUT;
        TIM_CCMR1(timer_peripheral) &= ~TIM_CCMR1_OC2M_MASK;
        switch (oc_mode) {
        case TIM_OCM_FROZEN:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_FROZEN;
            break;
        case TIM_OCM_ACTIVE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_ACTIVE;
            break;
        case TIM_OCM_INACTIVE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_INACTIVE;
            break;
        case TIM_OCM_TOGGLE:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_TOGGLE;
            break;
        case TIM_OCM_FORCE_LOW:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_FORCE_LOW;
            break;
        case TIM_OCM_FORCE_HIGH:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_FORCE_HIGH;
            break;
        case TIM_OCM_PWM1:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_PWM1;
            break;
        case TIM_OCM_PWM2:
            TIM_CCMR1(timer_peripheral) |= TIM_CCMR1_OC2M_PWM2;
            break;
        }
        break;
    case TIM_OC3:
        TIM_CCMR2(timer_peripheral) &= ~TIM_CCMR2_CC3S_MASK;
        TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_CC3S_OUT;
        TIM_CCMR2(timer_peripheral) &= ~TIM_CCMR2_OC3M_MASK;
        switch (oc_mode) {
        case TIM_OCM_FROZEN:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_FROZEN;
            break;
        case TIM_OCM_ACTIVE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_ACTIVE;
            break;
        case TIM_OCM_INACTIVE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_INACTIVE;
            break;
        case TIM_OCM_TOGGLE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_TOGGLE;
            break;
        case TIM_OCM_FORCE_LOW:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_FORCE_LOW;
            break;
        case TIM_OCM_FORCE_HIGH:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_FORCE_HIGH;
            break;
        case TIM_OCM_PWM1:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_PWM1;
            break;
        case TIM_OCM_PWM2:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC3M_PWM2;
            break;
        }
        break;
    case TIM_OC4:
        TIM_CCMR2(timer_peripheral) &= ~TIM_CCMR2_CC4S_MASK;
        TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_CC4S_OUT;
        TIM_CCMR2(timer_peripheral) &= ~TIM_CCMR2_OC4M_MASK;
        switch (oc_mode) {
        case TIM_OCM_FROZEN:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_FROZEN;
            break;
        case TIM_OCM_ACTIVE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_ACTIVE;
            break;
        case TIM_OCM_INACTIVE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_INACTIVE;
            break;
        case TIM_OCM_TOGGLE:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_TOGGLE;
            break;
        case TIM_OCM_FORCE_LOW:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_FORCE_LOW;
            break;
        case TIM_OCM_FORCE_HIGH:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_FORCE_HIGH;
            break;
        case TIM_OCM_PWM1:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_PWM1;
            break;
        case TIM_OCM_PWM2:
            TIM_CCMR2(timer_peripheral) |= TIM_CCMR2_OC4M_PWM2;
            break;
        }
        break;
    case TIM_OC1N:
    case TIM_OC2N:
    case TIM_OC3N:
        /* Ignoring as this option applies to the whole channel. */
        break;
    }
}

#define timer_set_oc_polarity_high(timer_peripheral, oc_id) timer_set_oc_polarity_high_cpp<timer_peripheral, oc_id>()
template <uint32_t timer_peripheral, enum tim_oc_id oc_id>
void timer_set_oc_polarity_high_cpp()
{
    switch (oc_id) {
    case TIM_OC1:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC1P;
        break;
    case TIM_OC2:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC2P;
        break;
    case TIM_OC3:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC3P;
        break;
    case TIM_OC4:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC4P;
        break;
    case TIM_OC1N:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC1NP;
        break;
    case TIM_OC2N:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC2NP;
        break;
    case TIM_OC3N:
        TIM_CCER(timer_peripheral) &= ~TIM_CCER_CC3NP;
        break;
    }
}

// For some reason these aren't defined inline??
// This saves time AND code
#define gpio_set(...) _gpio_set(__VA_ARGS__)
inline void _gpio_set(uint32_t gpioport, uint16_t gpios)
{
	GPIO_BSRR(gpioport) = gpios;
}

#define gpio_clear(...) _gpio_clear(__VA_ARGS__)
inline void  _gpio_clear(uint32_t gpioport, uint16_t gpios)
{
	GPIO_BSRR(gpioport) = (gpios << 16);
}

#endif
