#pragma once

#include <array>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

// Enable serial debug outputs
// constexpr auto DEBUG

// Enable 'optimized' versions of library functions in optimizations.h to save
// time and space
#define OPTIMIZATIONS

//#define DEBUG

// UART baud rate
constexpr auto UART_SPEED = 115200;

// Number of LEDs attached to MBI
constexpr auto NUM_LEDS = 11;

// Frames to draw per second
constexpr auto FPS = 60;

// Long presses are >= LONG_PRESS & < PWR_PRESS
constexpr size_t LONG_PRESS = FPS / 4; // ~250ms

// Power presses are >= PWR_PRESS
constexpr size_t PWR_PRESS = FPS * 3;

// Debounce delay after button input before accepting another input
constexpr size_t DEBOUNCE_DELAY = FPS / 20; // ~50ms

// LED chase order (zig-zag starting bottom right) - cap at NUM_LEDS for one
// direction NOTE TO SELF: Next time make the Dx numbers match the output pin
// numbers 🤦
constexpr std::array LED_ORDER
    = { 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 1U, 0U, 2U, 8U, 9U, 10U };

//
constexpr bool ENABLE_GAMMA = true;
// Gamma correction exponent
constexpr float GAMMA = 2.8;
// Gamma correction estimation degree/accuracy (only 3 supported for now)
constexpr int GAMMA_DEGREE = 3;

// Platform specific configurationf follows (pin/timer setup)

// Default hardware platform is STM32F0
#ifdef STM32F0
// IO layout:
//   PA0  : input/WKUP <- PWR switch
//   PA2  : TIM15_CH1  -> MBI5043 GCLK, 4MHz clock to LED driver
//   PA4  : output     -> MBI5043 LE, LED driver latch
//   PA5  : output     -> MBI5043 DCLK, LED driver serial clock
//   PA6  : output     -> Power switch for MBI5043
//   PA9  : USART1_TX  -> UART for application & bootloader
//   PA10 : USART1_RX  -> UART for application & bootloader
//   PA13 : output     -> MBI5043 SDI, LED driver serial data

constexpr auto GPIO_PORT = GPIOA;

constexpr auto PWR_SW = GPIO0;
constexpr auto PWR_SW_WKUP = PWR_CSR_EWUP1;

constexpr auto MBI_GCLK = GPIO2;
constexpr auto MBI_GCLK_TIMER = TIM15;
constexpr auto MBI_GCLK_TIMER_RCC = RCC_TIM15;

constexpr auto MBI_LE = GPIO4;
constexpr auto MBI_DCLK = GPIO5;

constexpr auto MBI_SDI = GPIO13;
constexpr auto MBI_PWR = GPIO6;

constexpr auto UART_TX = GPIO9;
constexpr auto UART_RX = GPIO10;

// Some defines are different on dev platform STM32F1
#elif STM32F1 // for dev on STM32F103 board
constexpr auto MBI_SDI = GPIO12;
constexpr auto MBI_GCLK = GPIO6;
constexpr auto MBI_GCLK_TIMER = TIM3;
constexpr auto MBI_GCLK_TIMER_RCC = RCC_TIM3;

constexpr auto GPIO_PORT = GPIOA;

constexpr auto PWR_SW = GPIO0;
constexpr auto PWR_SW_WKUP = PWR_CSR_EWUP;

constexpr auto MBI_LE = GPIO4;
constexpr auto MBI_DCLK = GPIO5;

constexpr auto MBI_PWR = GPIO6;

constexpr auto UART_TX = GPIO9;
constexpr auto UART_RX = GPIO10;
#endif