#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// Very slow at runtime, but used  for compile-time calculation of exp
#include "gcem.hpp"

#include "util.h"

using std::array;

// C++17 still doesn't define this...
constexpr double pi = 3.14159265358979323846;

// Way over the top implementation of a minimax coefficient estimator to
// determine Chebyshev coefficients for an efficient gamma correction
// implementation (basically a fast pow(x,y) over [0,1] for fixed y)
struct ChebyshevFit {

    // Get the Chebyshev node i with n total nodes
    static constexpr double cheby(double i, double n)
    {
        auto val = ((2.0 * i - 1) / (2.0 * n)) * pi;
        return gcem::cos(val) * -1;
    }

    // Scale x values over [a,b] to u values over [-1,1]
    template <class T> constexpr static T u_to_x(T u, T a, T b)
    {
        return u * (b - a) / 2 + (a + b) / 2;
    }
    // Scale u values over [-1,1] to x values over [a,b]
    template <class T> constexpr static T x_to_u(T x, T a, T b)
    {
        return (2 * x - a - b) / (b - a);
    }

    template <class T, class Func>
    static constexpr std::array<T, 4> fit(Func f, T a, T b)
    {
        // NOTE: not generalized for other degrees
        constexpr auto degree = 3;
        std::array<T, degree + 1> coeffs { 0, 0, 0, 0 };

        // Get the first degree+1 Chebyshev nodes' u values
        std::array<T, coeffs.size()> N {};
        for (auto i = 0U; i < N.size(); i++)
            N[i] = cheby(i + 1, N.size());

        // Get the x-scaled y values for those Chebyshev nodes
        std::array<T, N.size()> y { 0, 0, 0, 0 };
        for (auto i = 0U; i < N.size(); i++)
            y[i] = f(u_to_x(N[i], a, b));

        // coeffs[0] is the average of y
        for (auto val : y) {
            coeffs[0] += val;
        }
        coeffs[0] /= N.size();

        // coeffs[1] is 2 * avg(u*y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[1] += N[i] * y[i];
        }
        coeffs[1] *= 2.0 / N.size();

        // coeffs[2] is 2 * avg((2*u*u -1) * y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[2] += (2 * N[i] * N[i] - 1) * y[i];
        }
        coeffs[2] *= 2.0 / N.size();

        // coeffs[3] is 2 * avg((4*u*u*u - 3*u) * y)
        for (auto i = 0U; i < N.size(); i++) {
            coeffs[3] += (4 * N[i] * N[i] * N[i] - 3 * N[i]) * y[i];
        }
        coeffs[3] *= 2.0 / N.size();

        return coeffs;
    }
};

template <uint8_t n_leds, uint32_t gclk_timer, rcc_periph_clken gclk_timer_rcc>
class MBI5043 {
public:
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

    static constexpr uint16_t STARTUP_CONFIG = 0b0000001010110000;

    static constexpr uint16_t LED_MAX = 0xffff;
    static constexpr uint16_t LED_MIN = 0x0000;

    using fb_t = array<uint16_t, n_leds>;

    MBI5043(uint32_t port, uint32_t le_pin, uint32_t clk_pin, uint32_t data_pin)
        : _port(port)
        , _le_pin(le_pin)
        , _clk_pin(clk_pin)
        , _data_pin(data_pin)
    {
        config = STARTUP_CONFIG;
        cur_fb = 0;
        timer_setup();
    }

    // Get a buffer to draw (the next frame) to
    fb_t& get_buffer() { return buffers[cur_fb ^ 1]; }
    // Get the buffer containing the last/currently drawing frame
    const fb_t& cur_frame() const { return buffers[cur_fb]; }

    // Write the cur_fb buffer to the LEDs. Application hasn't had access
    // to this since the last frame. Current implemetnation of gamma correction
    // takes about 1ms/frame
    template <bool gamma_corrected = true> void put_frame()
    {
        auto& fb = buffers[cur_fb];
        if (gamma_corrected) {
            // Use the 'draw buffer' to write the output pixel data instead of
            // corrupting what the effects have generated
            gpio_set(GPIO_PORT, MBI_SDI);
            std::transform(
                fb.begin(), fb.end(), buffers[2].begin(), apply_gamma);
#ifdef DEBUG
            // char buf[128];
            // snprintf(buf, 128, "\n\nbefore:\n\t");
            // uart_writes(buf);
            // for (auto i = 0; i < fb.size(); i++) {
            //     snprintf(buf, 128, "%5hu ", fb[i]);
            //     uart_writes(buf);
            // }
            // snprintf(buf, 128, "\nafter:\n\t");
            // uart_writes(buf);
            // for (auto i = 0; i < fb.size(); i++) {
            //     snprintf(buf, 128, "%5hu ", buffers[2][i]);
            //     uart_writes(buf);
            // }
#endif
            gpio_clear(GPIO_PORT, MBI_SDI);
            // The rest of the code then draws the 'draw buffer'
            fb = buffers[2];
        }

        // Start with all lines low
        gpio_clear(_port, _le_pin | _clk_pin | _data_pin);

        // Data is written to the 'highest' port first, so we need to write some
        // zeroes if N_LEDS is not an integer multiple of 16, and then reverse
        // the order for the remainder
        for (auto i = 0; i < 16 - (n_leds % 16); i++)
            put_word(0);
        for (auto i = fb.rbegin(); i != fb.rend(); i++)
            put_word(*i);

        // Then we write an empty word with LE asserted for the last 3 clocks to
        // call for latch into the output comparators
        put_word(0, 3);

        // And finally swap buffers for the next frame
        cur_fb ^= 1;
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
    // allocate two buffers, a 'front buffer', a 'back buffer', and a 'draw
    // buffer' if we are doing gamma. Total bytes of memory will be N_LEDS * 4
    array<fb_t, ENABLE_GAMMA ? 3 : 2> buffers;

    volatile uint8_t cur_fb; // this gets changed in interrupt context
                             // (put_frame) so needs to be volatile

    uint32_t _port, _le_pin, _clk_pin, _data_pin;
    uint16_t config;

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

        timer_set_mode(
            gclk_timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        timer_set_prescaler(gclk_timer, 0);
        timer_disable_preload(gclk_timer);
        timer_set_period(gclk_timer, 1);
        timer_continuous_mode(gclk_timer);

        timer_set_oc_value(gclk_timer, TIM_OC1, 1);
        timer_set_oc_mode(gclk_timer, TIM_OC1, TIM_OCM_PWM1);
        timer_set_oc_polarity_high(gclk_timer, TIM_OC1);
        timer_enable_oc_output(gclk_timer, TIM_OC1);

        rcc_periph_clock_disable(gclk_timer_rcc);
    }
    // static inline double fastPrecisePow(double a, double b)
    // {
    //     // calculate approximation with fraction of the exponent
    //     int e = (int)b;
    //     union {
    //         double d;
    //         int x[2];
    //     } u = { a };
    //     u.x[1] = (int)((b - e) * (u.x[1] - 1072632447) + 1072632447);
    //     u.x[0] = 0;

    //     // exponentiation by squaring with the exponent's integer part
    //     // double r = u.d makes everything much slower, not sure why
    //     double r = 1.0;
    //     while (e) {
    //         if (e & 1) {
    //             r *= a;
    //         }
    //         a *= a;
    //         e >>= 1;
    //     }

    //     return r * u.d;
    // }

    // static inline float powf_fast(float a, float b)
    // {
    //     union {
    //         float d;
    //         int x;
    //     } u = { a };
    //     u.x = (int)(b * (u.x - 1064866805) + 1064866805);
    //     return u.d;
    // }

    // static uint16_t apply_gamma(const uint16_t val)
    // {
    //     return std::pow(float(val) / 65535, 2.8) * 65535 + 0.5;
    //     // return gamma_coeffs[0] + gamma_coeffs[1] * val;
    // }

    // Runtime ~1.6ms / frame or about 10% of frame time. Expensive in space
    // (3KB?) due to pulling in the FP functions.
    static uint16_t apply_gamma(const uint16_t val)
    {
        if (val == 0)
            return 0; // Off is off
        // Transform from int16 range to [0,1]
        float u = ChebyshevFit::x_to_u(float(val) / UINT16_MAX, 0.0f, 1.0f);
        float y = (gamma_coeffs[0] + gamma_coeffs[1] * u
            + gamma_coeffs[2] * (2 * u * u - 1)
            + gamma_coeffs[3] * (4 * u * u * u - 3 * u));
        // The approximation can (and does) return values < 0 and > 1, so
        // truncate cleanly
        if (y > 1.0)
            return UINT16_MAX;
        else if (y < 0.0)
            return 0;
        else
            return y * 65535;
    }

public:
    constexpr static array<float, 4> gamma_coeffs = ChebyshevFit::fit<float>(
        [](float x) -> float { return pow(x, GAMMA); }, 0, 1);
};