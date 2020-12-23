# Development Environment

## Software Dev Env

The project is based on [PlatformIO](https://platformio.org/), which should more or less do everything for you magically. I recommend the [VSCode integration](https://platformio.org/platformio-ide).

1. Install PlatformIO (ie. the VSCode extension of the same name)
2. Clone the repository
3. `git submodule update --init`
4. Open the `code` directory in VSCode
5. PlatformIO will install a bunch of stuff under the covers, but eventually you should be able to successfully execute the `PlatformIO: Build` command in VSCode.

## Programming the flash

The simplest way to program the hardware is using the serial bootloader, and this is how the PlatformIO project is setup. You will need:

* USB-serial converter capable of 3.3V levels (easily had on AliExpress or eBay for ~$1)
* Some Dupont-style hookup wires. Probably you will want male-female ones.
* Optionally pin headers to solder on the board.

Edit `platformio.ini` and look for `upload_port`. Set this to the appropriate port for your platform & serial converter (e.g. `COM3` or `/dev/ttyUSB0`).

1. Hook it up like this (stupidly I didn't mark the pins on the PCB): ![serial connection](../doc/serial%20connection.png)
2. Boot into the bootloader - to do this either hold the PROG button while applying power, or hold the PROG button while shorting RESET to GND (use a wire) and letting float. The LEDs should stay off.
3. Execute `PlatformIO: Upload` in VSCode

## Debugging

The SWD header can be used with an ST-Link debugger. Ostensibly, anyway. I wasn't able to get it to work when the microcontroller is in sleep mode (which is almost all of the time). PlatformIO supports this and I did use it successfully with the STM32F103 on the board I was using for dev before the PCBs came in.

In `config.h` there is a `DEBUG` define, if this is set non-zero, the serial port will also be initialized after boot, which you can open with a normal terminal application at 115200bps 8N1. The `debug_str` function will be available if `DEBUG` is defined, to print simple strings to the serial port for debugging.

# Architecture

Execution is driven by the Cortex M0 SysTick timer that ticks at 1/60s. When this timer fires, the interrupt handler has the MBI5043 driver write the buffer to the LED array (in interrupt context), so the framerate should be pretty tightly timed. A flag is set, and execution returns to the main loop. The main loop calls out to the effect to draw the next frame, then handles button input. When its work is done, it puts the microcontroller to sleep, waiting for the next SysTick interrupt. For power off, mainloop returns, and the processor is put into 'deep sleep' standby mode. On wakeup from this mode, the processor will be totally reset, so it will be identical to booting from fresh.

Effects are implemented as sub-classes of `MBIEffect`. The only required member function is `void operator(MBI& mbi, const uint32_t frames)`. This function receives a reference to the MBI5043 driver, and the current frame counter. It should call `mbi.get_buffer()` to get a reference to an array of `uint16_t` representing the LEDs, and modify it as appropriate. Values should span the full `uint16_t` range; they will be scaled and gamma corrected at output time.

![LED buffer indices](../doc/led%20indices.png)

After implementing your effect class, it must be instantiated (in `EffectSetup.h`) and included in the `effects` array found in `main.cpp`.