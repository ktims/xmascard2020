#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include <libopencm3/cm3/nvic.h>

#include <libopencm3/cm3/systick.h>

#include "config.h"
#include "optimizations.h"

#include "EffectSetup.h"
#include "MBI5043.h"
#include "util.h"

#ifdef DEBUG
#include <cstdio>
#endif

// MBI5043 LED driver instance
mbi_t mbi(GPIO_PORT, MBI_LE, MBI_DCLK, MBI_SDI, LED_OUT_MAX);

// Frame counter
uint32_t frame = 0;

// Auto power off when we get to this frame
uint32_t apo_frame = APO_FRAMES;

// Signal between interrupt-driven frame drawing and main loop when it's time to draw the next frame. This variable is
// stupidly named, when true it represents that a frame has been written to the LEDs and the buffer is ready for the
// next one
volatile bool frame_drawn = true;

// Enabled effects
const std::array<effect_ref, 5> effects = {
    TwinkleBlinkle,
    RandomSequence,
    ChaseRandom,
    TwinkleTwinkle,
    ChaseAround,
};

uint8_t cur_effect = 0;
uint8_t cur_bright = 6;

// This is a reference to the MBIEffect instance that will be drawn in the mainloop, it may differ from cur_effect for
// menus etc.
auto draw_frame = effects[0];

enum menu_state_t { MAIN, BRIGHT };

// Set up system clocks
void clock_setup()
{
    rcc_periph_clock_enable(RCC_GPIOA);
    systick_set_frequency(FPS, F_CPU);
}

// Set up GPIO & alternate functions
void io_setup()
{
    // The libopencm3 API is slightly different for different processor families.
#ifdef STM32F0
    // Set up GPIO outputs
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);
    gpio_set_output_options(GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);

    // Set up GPIO inputs with pull-down
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, PWR_SW);

    // Set up USART IOs
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_RX | UART_TX);
    gpio_set_af(GPIO_PORT, GPIO_AF1, UART_TX | UART_RX);

    // Set up GCLK output
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MBI_GCLK);
    gpio_set_af(GPIO_PORT, GPIO_AF4, MBI_GCLK);

#elif STM32F1
    rcc_periph_clock_enable(RCC_AFIO);
    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);

    gpio_set_mode(GPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, PWR_SW);

    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, UART_TX);
    gpio_set_mode(GPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, UART_RX);

    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, MBI_GCLK);

#else
#error "No GPIO setup for this STM32 family (or undefined family macro)"
#endif
}

#if DEBUG > 0
// Set up USART1 to transmit debug messages at 115200bps
void uart_setup()
{
    rcc_periph_clock_enable(RCC_USART1);
    usart_set_baudrate(USART1, UART_SPEED);
    usart_set_databits(USART1, 8);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    usart_enable(USART1);
}
#else
void uart_setup() { }
#endif

// Set the GPIO that controls the PFET power switch to MBI5043
// This device has a documented quiescent current of multiple mA, which is way too high when we need a standby current
// of µA when off.
void mbi_power(bool on)
{
    if (on)
        gpio_clear(GPIO_PORT, MBI_PWR);
    else
        gpio_set(GPIO_PORT, MBI_PWR);
}

// SysTick ISR
// When we hit the frame timer, and there's a frame ready for us, draw it and signal back
void sys_tick_handler(void)
{
    if (!frame_drawn) {
        mbi.put_frame<ENABLE_GAMMA>();
        frame_drawn = true;
    }
    // Increment regardless of whether we actually drew a frame to the buffer, this is our timekeeping
    frame++;
}

void board_init()
{
    clock_setup();
    io_setup();
    uart_setup();

    mbi_power(true);

    nvic_enable_irq(NVIC_SYSTICK_IRQ);
    systick_clear();
    systick_interrupt_enable();
    systick_counter_enable();
}

void set_effect(const uint8_t i)
{
    cur_effect = i % effects.size();
    draw_frame = effects[cur_effect];
    mbi.clear_buffers();
}

menu_state_t press_main()
{
    set_effect(cur_effect + 1);
    return MAIN;
}
menu_state_t press_bright()
{
    cur_bright = cur_bright == 0 ? 6 : cur_bright - 1;
    mbi.bright = (static_cast<uint32_t>(1) << (cur_bright + 9)) - 1;

    indicator.n = cur_bright + 1;
    draw_frame = indicator;
    return BRIGHT;
}

menu_state_t long_press_main()
{
    indicator.n = cur_bright + 1;
    draw_frame = indicator;
    return BRIGHT;
}
menu_state_t long_press_bright()
{
    set_effect(cur_effect);
    return MAIN;
}

// Called after initialization. To 'request' a power off, mainloop returns, and the caller will power off the CPU or
// whatever
void mainloop()
{
    enum sw_state_t { WAITING, PRESSED, RELEASED, POWER_OFF_DELAY };

    sw_state_t sw_state = WAITING;
    menu_state_t menu_state = MAIN;
    size_t frames_held = 0;

    while (true) {
        if (frame_drawn) {
            draw_frame(mbi, frame);
            frame_drawn = false;
        }

        // Do button i/o lazily in the main loop.
        // This breaks badly if processing a frame takes longer than 1 frame, so don't do that
        bool sw = gpio_get(GPIO_PORT, PWR_SW);

        if (sw)
            apo_frame = frame + APO_FRAMES;

        if (frame == apo_frame) {
            // it's possible that we miss this, if frame generation is taking too long but doing >= instead means we
            // have to handle overflow
            return;
        }

        switch (sw_state) {
        case WAITING:
            if (sw) {
                sw_state = PRESSED;
                frames_held = 1;
            }
            break;
        case PRESSED:
            // If the switch is still held, don't act yet, but update the display to show we received the input
            if (sw) {
                frames_held++;
                if (frames_held >= PWR_PRESS) {
                    // Turn off all the LEDs to indicate we are about to turn off
                    draw_frame = AllOff;
                } else if (frames_held > LONG_PRESS) {
                    // Turn on one LED to indicate we received the long-press
                    indicator.n = 1;
                    draw_frame = indicator;
                }
            } else {
                if (frames_held <= LONG_PRESS) {
                    debug_str("Short press detected\n");
                    switch (menu_state) {
                    case MAIN:
                        menu_state = press_main();
                        break;
                    case BRIGHT:
                        menu_state = press_bright();
                        break;
                    }
                } else if (frames_held >= PWR_PRESS) {
                    // I would rather this happened immediately when the button has been held for a few
                    // seconds, but this causes the processor to be unable to wake up
                    debug_str("Waiting to standby\n");
                    sw_state = POWER_OFF_DELAY;

                    break;
                } else {
                    debug_str("Long press detected\n");
                    switch (menu_state) {
                    case MAIN:
                        menu_state = long_press_main();
                        break;
                    case BRIGHT:
                        menu_state = long_press_bright();
                        break;
                    }
                }
                sw_state = RELEASED;
                frames_held = 0;
            }
            break;
        case RELEASED:
            frames_held++;
            // DEBOUNCE_DELAY dead frames before we accept button presses again
            if (frames_held >= DEBOUNCE_DELAY) {
                sw_state = WAITING;
                frames_held = 0;
            }
            break;

        case POWER_OFF_DELAY:
            frames_held++;
            // Wait DEBOUNCE_DELAY frames after PWR_SW released to make sure it has settled before powering off
            if (frames_held >= DEBOUNCE_DELAY) {
                debug_str("Mainloop returning to enter standby\n");
                return;
            }
            break;
        }

        // Sleep until the next frame interrupt
        cpu_sleep();
    }
}

int main(void)
{
    debug_str("main entered\n");
    board_init();
    debug_str("board inited\n");

#if DEBUG > 0
    char buf[128];
    snprintf(buf, 128, "\n\nGamma approximation coefficients (float32):\n");
    debug_str(buf);
    for (auto i = 0U; i < mbi.gamma_coeffs.size(); i++) {
        const unsigned char* c = (const unsigned char*)(&mbi.gamma_coeffs[i]);
        snprintf(buf, 128, "\t%i: 0x%02x%02x%02x%02x\n", i, *c, *(c + 1), *(c + 2), *(c + 3));
        debug_str(buf);
    }
#endif
    mbi.config = (mbi.config
                     & ~(_BV(mbi_t::GAIN_B0) | _BV(mbi_t::GAIN_B1) | _BV(mbi_t::GAIN_B2) | _BV(mbi_t::GAIN_B3)
                         | _BV(mbi_t::GAIN_B4) | _BV(mbi_t::GAIN_B5)))
        | (MBI_GAIN << 4);
    mbi.config |= _BV(mbi_t::GCLK_DDR);
    mbi_power(true);
    mbi.start();
    debug_str("MBI5043 started\n");

    effect_rng.seed(get_true_random);

    set_effect(std::uniform_int_distribution<uint8_t>(0, effects.size() - 1)(effect_rng));

    mainloop();

    mbi.stop();
    mbi_power(false);
    cpu_off(); // This will not return, since the device's registers and SRAM are reset after wakeup from deep sleep
    debug_str("Here be dragons\n");
}
