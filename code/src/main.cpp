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
mbi_t mbi(GPIO_PORT, MBI_LE, MBI_DCLK, MBI_SDI);

// Frame counter
// volatile uint32_t frame = 0;

// Signal between interrupt-driven frame drawing and main loop when it's time to
// draw the next frame
volatile bool frame_drawn = true;

// Enabled effects
const std::array<effect_ref, 4> effects
    = { AllOff, AllOn, AllRandom, ChaseAround };

// The current effect, initializer is the effect at boot
auto draw_frame = effects[0];

uint32_t frame = 0;

// Set up system clocks
void clock_setup()
{
    rcc_periph_clock_enable(RCC_GPIOA);
    systick_set_frequency(FPS, F_CPU);
}

// Set up GPIO & alternate functions
void io_setup()
{
#ifdef STM32F0
    // Set up GPIO outputs
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);
    gpio_set_output_options(GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
        MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);

    // Set up GPIO inputs with pull-down
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, PWR_SW);

    // Set up USART IOs
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, UART_RX | UART_TX);
    gpio_set_af(GPIO_PORT, GPIO_AF1, UART_TX | UART_RX);

    // Set up GCLK output
    gpio_mode_setup(GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MBI_GCLK);
    gpio_set_af(GPIO_PORT, GPIO_AF0, MBI_GCLK);

#elif STM32F1
    rcc_periph_clock_enable(RCC_AFIO);
    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
        MBI_DCLK | MBI_LE | MBI_PWR | MBI_SDI);

    gpio_set_mode(
        GPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, PWR_SW);

    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, UART_TX);
    gpio_set_mode(GPIO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, UART_RX);

    gpio_set_mode(GPIO_PORT, GPIO_MODE_OUTPUT_10_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, MBI_GCLK);

#else
#error "No GPIO setup for this STM32 family (or undefined family macro)"
#endif
}

#ifdef DEBUG
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
#endif

// Set the GPIO that controls the PFET power switch to MBI5043
// This device has a documented quiescent current of multiple mA, which is way
// too high when we need a standby current of µA
void mbi_power(bool on)
{
    if (on)
        gpio_clear(GPIO_PORT, MBI_PWR);
    else
        gpio_set(GPIO_PORT, MBI_PWR);
}

// When we hit the frame timer, and there's a frame ready for us, draw it and
// signal back
void sys_tick_handler(void)
{
    if (!frame_drawn) {
        mbi.put_frame<ENABLE_GAMMA>();
        frame_drawn = true;
    }
    // Increment regardless of whether we actually drew a frame to the buffer
    frame++;
}

void board_init()
{
    clock_setup();
    io_setup();
    uart_setup();

    mbi_power(true);

    nvic_enable_irq(NVIC_SYSTICK_IRQ);
    systick_interrupt_enable();
    systick_counter_enable();
}

void press() { draw_frame = effects[1]; }

void long_press(size_t frames_held) { draw_frame = effects[2]; }

// Called after initialization. State that needs to be reset on power off will
// be in its stack frame. To 'request' a power off, mainloop returns, and the
// outer loop will power off the CPU and call it fresh when power is restored.
void mainloop()
{
    enum sw_state_t { WAITING, PRESSED, RELEASED, POWER_OFF_DELAY };
    sw_state_t sw_state = WAITING;
    size_t frames_held = 0;
    // TODO: Mainloop will sleep after drawing a frame to the buffer. SysTick
    // will wake the CPU when it's frame time, push the frame to the LEDs, and
    // return to mainloop which will draw the next frame.
    while (true) {
        if (frame_drawn) {
            draw_frame(mbi, frame);
            frame_drawn = false;
        }

        // Do button i/o lazily in the main loop. <=~1/10s = short press,
        // anything longer is a long press. Power down after a 1s press.
        // This breaks badly if processing a frame takes longer than 1 frame, or
        // sleep is disabled
        bool sw = gpio_get(GPIO_PORT, PWR_SW);

        switch (sw_state) {
        case WAITING:
            if (sw) {
                sw_state = PRESSED;
                frames_held = 1;
            }
            break;
        case PRESSED:
            // If the switch is still held, don't do anything yet. Wait for
            // release.
            if (sw)
                frames_held++;
            else {
                if (frames_held <= LONG_PRESS) {
                    debug_str("Short press detected\n");
                    press();
                } else if (frames_held >= PWR_PRESS) {
                    debug_str("Waiting to standby\n");
                    sw_state = POWER_OFF_DELAY;
                    frames_held = 0;
                    break;
                } else {
                    debug_str("Long press detected\n");
                    long_press(frames_held);
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
            // Wait DEBOUNCE_DELAY frames after PWR_SW released to make sure it
            // has settled before powering off
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
    board_init();
#ifdef DEBUG
    char buf[128];
    snprintf(buf, 128, "\n\nGamma approx. coefficients (float32):\n");
    debug_str(buf);
    for (auto i = 0U; i < mbi.gamma_coeffs.size(); i++) {
        const unsigned char* c = (const unsigned char*)(&mbi.gamma_coeffs[i]);
        snprintf(buf, 128, "\t%i: 0x%02x%02x%02x%02x\n", i, *c, *(c + 1),
            *(c + 2), *(c + 3));
        debug_str(buf);
    }
#endif

    mbi_power(true);
    mbi.start();

    mainloop();

    mbi.stop();
    mbi_power(false);
    cpu_off(); // This will not return, since the device's registers and
    // SRAM are reset after deep sleep
    debug_str("Here be dragons\n");
}