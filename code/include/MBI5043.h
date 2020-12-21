#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "ChebyshevFit.h"
#include "util.h"

using std::array;

template <uint8_t n_leds, uint32_t gclk_timer, rcc_periph_clken gclk_timer_rcc> class MBI5043 {
public:
    // Configuration bit positions
    static constexpr uint16_t GCLK_SHIFT_B1 = 15;
    static constexpr uint16_t GCLK_SHIFT_B0 = 14;
    static constexpr uint16_t PC_MODE_B1 = 12;
    static constexpr uint16_t PC_MODE_B0 = 11;
    static constexpr uint16_t CS_MODE_BA = 10;
    static constexpr uint16_t CS_MODE_BB = 2;
    static constexpr uint16_t GAIN_B5 = 9;
    static constexpr uint16_t GAIN_B4 = 8;
    static constexpr uint16_t GAIN_B3 = 7;
    static constexpr uint16_t GAIN_B2 = 6;
    static constexpr uint16_t GAIN_B1 = 5;
    static constexpr uint16_t GAIN_B0 = 4;
    static constexpr uint16_t GCLK_DDR = 3;
    static constexpr uint16_t ENABLE = 0;

    // Startup configuration register state.
    static constexpr uint16_t STARTUP_CONFIG = 0b0000001010110000;

    static constexpr uint16_t LED_MAX = 0xffff;
    static constexpr uint16_t LED_MIN = 0x0000;

    static constexpr auto N_LEDS = n_leds;

    using fb_t = array<uint16_t, n_leds>;

    uint16_t bright;
    uint16_t config;

    MBI5043(uint32_t port, uint32_t le_pin, uint32_t clk_pin, uint32_t data_pin, uint16_t brightness)
        : bright(brightness)
        , _port(port)
        , _le_pin(le_pin)
        , _clk_pin(clk_pin)
        , _data_pin(data_pin)
    {
        config = STARTUP_CONFIG;
        timer_setup();
    }

    // Get a buffer to draw (the next frame) to
    fb_t& get_buffer() { return buffers[0]; }
    // Get the buffer containing the next frame (which will be the frame previous to what is about to be drawn into
    // get_buffer)
    const fb_t& cur_frame() const { return buffers[0]; }

    // Clear all buffers (new effect)
    void clear_buffers()
    {
        for (auto& b : buffers) {
            std::fill(b.begin(), b.end(), 0);
        }
    }

    // Write the buffer to the LEDs.
    // Current implementation of gamma correction takes about 1.5ms/frame @ 8MHz
    template <bool gamma_corrected = true> void put_frame()
    {
        // Draw from this buffer, 'corrections' will write to it
        auto& fb = buffers[1];

        std::transform(buffers[0].begin(), buffers[0].end(), fb.begin(),
            [this](uint16_t val) { return apply_correction<gamma_corrected>(val); });

        // Start with all lines low
        gpio_clear(_port, _le_pin | _clk_pin | _data_pin);

        // Data is written to the 'highest' port first, so we need to write some zeroes if N_LEDS is not an integer
        // multiple of 16, and then reverse the order for the remainder
        for (auto i = 0; i < 16 - (n_leds % 16); i++)
            put_word(0);
        for (auto i = fb.rbegin(); i != fb.rend(); i++)
            put_word(*i);

        // Then we write an empty word with LE asserted for the last 3 clocks to call for latch into the output
        // comparators
        put_word(0, 3);
    }

    void put_config() const
    {
        // Enable writing configuration
        put_word(0, 15);
        put_word(config, 11);
    }

    void start()
    {
        config |= (1 << ENABLE);
        put_config();
        rcc_periph_clock_enable(gclk_timer_rcc);
        timer_enable_counter(gclk_timer);
    }

    void stop()
    {
        config &= ~(1 << ENABLE);
        put_config();
        timer_disable_counter(gclk_timer);
        rcc_periph_clock_disable(gclk_timer_rcc);
    }

private:
    // one buffer for the frame as generated, one for the gamma-corrected and scaled output
    array<fb_t, 2> buffers;

    uint32_t _port, _le_pin, _clk_pin, _data_pin;

    void put_word(const uint16_t w, const uint8_t latch_clocks = 1) const
    {
        // mask each bit, starting with MSB
        for (auto i = 0x8000; i > 0; i >>= 1) {
            // set clk low for bit setup
            gpio_clear(_port, _clk_pin);
            // Raise LE for the last <latch_clocks> periods
            if (i == (1 << (latch_clocks - 1)))
                gpio_set(_port, _le_pin);
            // put the bit on data_pin
            (w & i) ? gpio_set(_port, _data_pin) : gpio_clear(_port, _data_pin);
            // toggle a rising edge on clk
            gpio_set(_port, _clk_pin);
        }
        gpio_clear(_port, _le_pin | _clk_pin);
    }

    void timer_setup() const
    {
        rcc_periph_clock_enable(gclk_timer_rcc);

        timer_set_mode(gclk_timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        timer_set_prescaler(gclk_timer, 1);
        timer_disable_preload(gclk_timer);
        timer_set_period(gclk_timer, 1);
        timer_continuous_mode(gclk_timer);

        timer_set_oc_value(gclk_timer, TIM_OC1, 1);
        timer_set_oc_mode(gclk_timer, TIM_OC1, TIM_OCM_PWM1);
        timer_set_oc_polarity_high(gclk_timer, TIM_OC1);
        timer_enable_oc_output(gclk_timer, TIM_OC1);

        rcc_periph_clock_disable(gclk_timer_rcc);
    }

    // Runtime ~1.4ms / frame or about 8% of frame time. Pretty expensive in space.
    template <bool gamma_corrected = true> uint16_t apply_correction(const uint16_t val)
    {
        if (val == 0)
            return 0; // Off is off

        if (gamma_corrected) {
            // The following is equivalent to:>
            // float y = std::pow(float(val) * (1.0F / LED_MAX), GAMMA);

            // Transform from uint16 range to [-1,1]
            float u = ChebyshevFit::x_to_u(float(val) * (1.0F / LED_MAX), 0.0f,
                1.0f); // Using * here instead of / saves 500b of flash since we
                       // avoid pulling in fdiv that can't be optimized out
            float y = (gamma_coeffs[0] + gamma_coeffs[1] * u + gamma_coeffs[2] * (2 * u * u - 1)
                + gamma_coeffs[3] * (4 * u * u * u - 3 * u));

            // The approximation can (and does) return values < 0 and > 1, so
            // truncate cleanly
            if (y > 1.0)
                return bright;
            else if (y < 0.0)
                return 0;
            else
                return y * bright;
        } else {
            return val * bright;
        }
    }

public:
    static constexpr auto gamma_coeffs = ChebyshevFit::fit<float>(
        [](float x) constexpr->float { return gcem::pow(x, GAMMA); }, 0, 1);
};
