#pragma once

#include <string>

#ifdef DEBUG
#define debug_str(x) uart_writes(x)
#else
#define debug_str(x) x
#endif

void uart_setup();
void uart_writes(const std::string&);

void cpu_sleep();
void cpu_off();