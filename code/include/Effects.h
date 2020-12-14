
#pragma once
#include "MBIEffect.h"

// EFFECTS

// Trivial effects implemented with STL + lambdas

// Fill the buffer with <led_val>. Templated for the value since the
// function can't take parameters when wrapped in effect_prototype. A class
// version or using std::bind for every new value would be required to make it
// variable at runtime.
template <class MBI, uint16_t led_val> struct AllToValue : MBIEffect<MBI> {
    void operator()(MBI& mbi, uint32_t)
    {
        std::fill(mbi.get_buffer().begin(), mbi.get_buffer().end(), led_val);
    }
};

// Set a random value between lower_bound and upper_bound to each LED every
// frame. This is probably useless for anything other than testing the serial
// protocol.
template <class MBI, uint16_t lower_bound = MBI::LED_MIN,
    uint16_t upper_bound = MBI::LED_MAX>
struct AllRandomRange : MBIEffect<MBI> {
    void operator()(MBI& mbi, uint32_t)
    {
        static std::uniform_int_distribution<uint16_t> gen(
            lower_bound, upper_bound);
        std::generate(
            mbi.get_buffer().begin(), mbi.get_buffer().end(),
            []() -> auto { return gen(effect_rng); });
    }
};

// Chase around. Ramp up the target LED, ramp down the non-target LEDs.
template <class MBI, class Iter> struct Chase : MBIEffect<MBI> {
    enum Direction { UP = 1, DOWN = -1 };

    Direction _dir;

    // Speed is the number of frames to fade up the target LED before moving on.
    // Length is the number of LEDs in the 'tail' that are fading down.
    Chase(Iter ledmap_first, Iter ledmap_last, uint16_t speed = 20,
        uint16_t length = 4, Direction dir = UP)
        : _dir(dir)
        , first(ledmap_first)
        , last(ledmap_last)
        , _speed(speed)
        , _length(length)

    {
        pos = first;
        frames_left = speed;
        step_size = (MBI::LED_MAX - MBI::LED_MIN) / (speed * length);
    }
    void operator()(MBI& mbi, uint32_t)
    {
        auto& fb = mbi.get_buffer();
        const auto& cur = mbi.cur_frame();

        if (!frames_left) {
            if (_dir == UP) {
                pos++;
                if (pos == last)
                    pos = first;
            } else {
                if (pos == first)
                    pos = last;
                pos--;
            }
            frames_left = _speed;
        }

        // Loop the output buffer checking whether we should (without overflowing)
        // decrement or increment
        for (auto i = 0U; i < fb.size(); i++) {
            if (i != *pos)
                fb[i] = std::min<uint16_t>(cur[i], cur[i] - step_size);
            else
                // Ramp up the 'target' LED:
                fb[i] = std::max<uint16_t>(
                    cur[i], cur[i] + (step_size * _length));
        }
        frames_left--;
    }

private:
    Iter first, last, pos; // Changing these on the fly will break stuff
    uint16_t frames_left, step_size;
    uint16_t _speed, _length;
};