#pragma once

#include <random>
#include <type_traits>
#include <utility>

#include "MBIEffect.h"

// EFFECTS

// Trivial effects implemented with STL + lambdas

// Fill the buffer with <led_val>. Templated for the value since the function can't take parameters when wrapped in
// effect_prototype. A class version or using std::bind for every new value would be required to make it variable at
// runtime.
template <class MBI, uint16_t led_val> struct AllToValue : MBIEffect<MBI> {
    void operator()(MBI& mbi, const uint32_t) { std::fill(mbi.get_buffer().begin(), mbi.get_buffer().end(), led_val); }
};

// Set a random value between lower_bound and upper_bound to each LED every frame. This is probably useless for anything
// other than testing the serial protocol.
template <class MBI, uint16_t lower_bound = MBI::LED_MIN, uint16_t upper_bound = MBI::LED_MAX>
struct AllRandomRange : MBIEffect<MBI> {
    void operator()(MBI& mbi, const uint32_t)
    {
        static std::uniform_int_distribution<uint16_t> gen(lower_bound, upper_bound);
        std::generate(
            mbi.get_buffer().begin(), mbi.get_buffer().end(), []() -> auto { return gen(effect_rng); });
    }
};

// Ramp continuously by step_size
template <class MBI> struct RampAll : MBIEffect<MBI> {
    uint16_t step_size;
    RampAll(uint16_t _step_size)
        : step_size(_step_size)
    {
    }
    void operator()(MBI& mbi, const uint32_t)
    {
        std::transform(
            mbi.get_buffer().begin(), mbi.get_buffer().end(),
            mbi.get_buffer().begin(), [this](auto val) -> auto { return val + step_size; });
    }
};

// Chase around. Ramp up the target LED, ramp down the non-target LEDs.
template <class MBI, class Iter> struct Chase : MBIEffect<MBI> {
    enum Direction { UP = 1, DOWN = -1 };

    Direction _dir;

    // Speed is the number of frames to fade up the target LED before moving on.
    // Length is the number of LEDs in the 'tail' that are fading down.
    Chase(Iter ledmap_first, Iter ledmap_last, uint16_t speed = 20, uint16_t length = 4, Direction dir = UP)
        : _dir(dir)
        , first(ledmap_first)
        , last(ledmap_last)
        , _speed(speed)
        , _length(length)

    {
        pos = last;
        frames_left = speed;
        step_size = (MBI::LED_MAX - MBI::LED_MIN) / (speed * length);
    }
    void operator()(MBI& mbi, const uint32_t)
    {
        auto& fb = mbi.get_buffer();
        // const auto& cur = mbi.cur_frame();
        // RNG will be initialized by now, so on the first loop we choose a random start position
        if (pos == last)
            pos = first + std::uniform_int_distribution<uint8_t>(0, last - first - 1)(effect_rng);
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

        // Loop the output buffer checking whether we should (without overflowing) decrement or increment
        for (auto i = 0U; i < fb.size(); i++) {
            if (i != *pos) {
                // Ramp down the long tail
                // fb[i] -= std::min<uint16_t>(step_size, fb[i]);
                fb[i] = sat_sub(fb[i], step_size);
            } else {
                // Ramp up the 'target' LED _length times as fast
                // fb[i] += std::min<uint16_t>(step_size * _length, MBI::LED_MAX - fb[i]);
                fb[i] = sat_add(fb[i], step_size * _length);
            }
        }
        frames_left--;
    }

private:
    Iter first, last, pos; // Changing these on the fly will break stuff
    uint16_t frames_left, step_size;
    uint16_t _speed, _length;
};

// This and the ordered chase can probably be refactored into one class...
template <class MBI> struct RandomChase : MBIEffect<MBI> {

    // Speed is the number of frames to fade up the target LED before moving on.
    // Length is the number of LEDs in the 'tail' that are fading down.
    RandomChase(uint16_t speed = 20, uint16_t length = 4, uint16_t min_val = 16384)
        : _min_val(min_val)
        , _speed(speed)
        , _length(length)
    {
        pos = pos_map.begin();
        frames_left = speed;
        step_size = (MBI::LED_MAX - min_val) / (speed * length);

        // Start with an ordered sequence 0 - N_LEDS-1
        std::iota(pos_map.begin(), pos_map.end(), 0);

        // shuffle it
        new_posmap();
    }
    void operator()(MBI& mbi, const uint32_t)
    {
        auto& fb = mbi.get_buffer();

        if (!frames_left) {
            pos++;
            if (pos == pos_map.end()) {
                new_posmap();
                pos = pos_map.begin();
            }
            frames_left = _speed;
        }

        // Loop the output buffer checking whether we should (without overflowing) decrement or increment
        for (auto i = 0U; i < fb.size(); i++) {
            if (i != *pos) {
                // Ramp down the long tail
                // fb[i] -= std::min<uint16_t>(step_size, fb[i]);
                fb[i] = std::max(_min_val, sat_sub(fb[i], step_size));
            } else {
                // Ramp up the 'target' LED _length times as fast
                // fb[i] += std::min<uint16_t>(step_size * _length, MBI::LED_MAX - fb[i]);
                fb[i] = sat_add(fb[i], step_size * _length);
            }
        }
        frames_left--;
    }

private:
    void new_posmap() { std::shuffle(pos_map.begin(), pos_map.end(), effect_rng); }
    std::array<uint8_t, MBI::N_LEDS> pos_map;
    typename decltype(pos_map)::const_iterator pos;
    uint16_t frames_left, step_size, _min_val;
    uint16_t _speed, _length;
};

template <class MBI> struct Twinkle : MBIEffect<MBI> {

    Twinkle(int16_t magnitude, uint16_t speed)
    {
        val_gen = std::uniform_int_distribution<int16_t>(-magnitude, magnitude);
        frame_gen = std::uniform_int_distribution<uint16_t>(10, speed);
        for (auto& i : targets) {
            i = make_target(0);
        }
    }

    void operator()(MBI& mbi, const uint32_t frame)
    {
        auto& fb = mbi.get_buffer();

        for (auto i = 0U; i < fb.size(); i++) {
            auto t_v = std::get<0>(targets[i]);
            auto t_f = std::get<1>(targets[i]);
            if (frame >= t_f) { // reached the target frame, set to the target value and generate a new target
                fb[i] = t_v;
                targets[i] = make_target(frame);
            } else {
                fb[i] = sat_ofs(fb[i],
                    (t_v - fb[i]) / static_cast<int32_t>(t_f - frame)); // Add the distance to go over frames to go
            }
        }
    }

private:
    // target value, target frame
    using target_t = std::pair<uint16_t, uint32_t>;

    target_t make_target(const uint32_t frame)
    {
        auto val_ofs = val_gen(effect_rng);
        auto nf = frame + frame_gen(effect_rng);

        if (val_ofs >= 0)
            return target_t { sat_add(MBI::LED_MAX / 2, val_ofs), nf };
        else
            return target_t { std::max<uint16_t>(sat_ofs(MBI::LED_MAX / 2, val_ofs), MBI::LED_MAX / 10), nf };
    }

    std::uniform_int_distribution<int16_t> val_gen;
    std::uniform_int_distribution<uint16_t> frame_gen;
    std::array<target_t, MBI::N_LEDS> targets;
};

template <class MBI> struct FirstN : MBIEffect<MBI> {
    uint8_t n;
    bool on;

    FirstN(const uint8_t _n, const bool _on)
        : n(_n)
        , on(_on)
    {
    }

    void operator()(MBI& mbi, const uint32_t)
    {
        std::fill(mbi.get_buffer().begin(), mbi.get_buffer().begin() + n, on ? mbi.LED_MAX : mbi.LED_MIN);
        std::fill(mbi.get_buffer().begin() + n, mbi.get_buffer().end(), on ? mbi.LED_MIN : mbi.LED_MAX);
    }
};
