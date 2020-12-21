#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "config.h"
#include "util.h"

// Write a string to USART1
void uart_writes(const std::string& str)
{
    for (auto c : str) {
        if (c == '\n')
            usart_send_blocking(USART1, '\r');
        usart_send_blocking(USART1, c);
    }
}

// For some reason libopencm3 doesn't seem to provide this. Call the 'wait for
// interrupt' operation to put the cpu to sleep.
void cpu_sleep() { asm("wfi"); }

// Put the CPU to deep sleep, basically off. It will come back on with a POR.
// Most of this is not well covered by libopencm3 afaict.
void cpu_off()
{
    rcc_periph_clock_enable(RCC_PWR);
    PWR_CR &= PWR_CR_LPDS;
    PWR_CR |= PWR_CR_PDDS;
    PWR_CSR |= PWR_SW_WKUP;
    PWR_CR |= PWR_CR_PDDS | PWR_CR_CWUF;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    systick_counter_disable();
    nvic_disable_irq(NVIC_SYSTICK_IRQ);

    asm("wfi");
}

// Use the ADC to get a non-deterministic value, that will be used to seed the RNG so we can have a random effect at
// startup. Collects ADC values, uses von Neumann de-biasing on the bottom 2 (noisiest) bits to generate a 32-bit value.
// Probably not good enough for crypto, but certainly good enough to randomly choose 1 of 5 effects.
uint32_t get_true_random()
{
    using result_t = uint32_t;
    result_t ret = 0;
    uint8_t bits_left = sizeof(result_t) * 8;

    rcc_periph_clock_enable(RCC_ADC);

    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

    adc_power_off(ADC);
    adc_set_clk_source(ADC, ADC_CLKSOURCE_ADC);
    adc_calibrate(ADC);

    adc_set_operation_mode(ADC, ADC_MODE_SCAN);
    adc_disable_external_trigger_regular(ADC);
    adc_set_right_aligned(ADC);
    adc_set_sample_time_on_all_channels(ADC, ADC_SMPTIME_001DOT5);
    uint8_t chans[] = { 1, 1, 9 };
    adc_set_regular_sequence(ADC, 1, chans);
    adc_set_resolution(ADC, ADC_RESOLUTION_12BIT);
    adc_disable_analog_watchdog(ADC);

    adc_power_on(ADC);

    // Remove bias
    while (bits_left) {
        adc_start_conversion_regular(ADC);
        while (!adc_eoc(ADC))
            ;
        auto val = adc_read_regular(ADC) & 0x03; // get only the bottom 2 bits
        if (val == 1) { // bottom bits are 01, shift a 0 in
            ret = (ret << 1) | 0;
            bits_left--;
        } else if (val == 2) { // bottom bits are 10, shift a 1 in
            ret = (ret << 1) | 1;
            bits_left--;
        }
    }

    adc_power_off(ADC);
    rcc_periph_clock_disable(RCC_ADC);
    return ret;
}