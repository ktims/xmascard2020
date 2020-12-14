#pragma once

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#include "MBI5043.h"
#include "config.h"
#include "rng.h"

// DEFINITIONS

// Why not use dynamic dispatch for something like this
template <class MBI>
struct MBIEffect {
    virtual void operator()(MBI& mbi, uint32_t frames) = 0;
};

// HELPERS / GLOBALS

// Let's have ourselves a global RNG for all effects to use
auto effect_rng = pcg();

// Fade to a target buffer state over a given number of frames (linearly
// interpolate). Will interpolate the next frame and store it into `out`. It is
// intended for the calling effect to set up the buffer in its own static
// storage, with the target frame number, and delegate to this function until
// the target frame is reached, then generate the next target frame etc.
template <class effect_fb_t>
void fade_to_frame(effect_fb_t out, const effect_fb_t cur,
    const effect_fb_t target, uint32_t nframes)
{
    for (auto i = 0U; i < out.size(); i++) {
        // How far left to go, divided by how many frames we have to do it, plus
        // the current value
        out[i] += (target[i] - cur[i]) / nframes;
    }
}
