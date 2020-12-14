#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/scb.h>

#include "util.h"
#include "config.h"

#ifdef DEBUG
// Write a string to USART1
void uart_writes(const std::string& str)
{
    for (auto c : str) {
        if (c == '\n')
            usart_send_blocking(USART1, '\r');
        usart_send_blocking(USART1, c);
    }
}
#else
// Implement stubs that the optimizer will remove
void uart_setup() { }
void uart_writes(const std::string&) { }
#endif


// For some reason libopencm3 doesn't seem to provide this. Call the 'wait for
// interrupt' operation to put the cpu to sleep.
void cpu_sleep() { asm("wfi"); }

// Put the CPU to deep sleep, basically off. It will come back on with a POR.
// Most of this is not well covered by libopencm3.
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