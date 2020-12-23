# Merry Christmas 2020! 🎄🎁

If you're here, it's probably because I sent you a card/ornament and you can't get it to work, thought it might be fun to hack on, or you were curious how it was designed. Unless it's the first one, awesome!!

This was a vague idea in my head on Dec 4th, sent for manufacturing on Dec 6th, and if I managed to get it to you before Christmas, considering my projects usually go on for years and never really get 'finished', I'm super impressed with myself. Guys, I actually finished a project!! It's been a 'sprint'! I hope you enjoy it 😁.

If you just want to get hacking, download [PlatformIO](https://platformio.org/platformio-ide), check out the repo, and have a look at `code/include/Effects.h`. The only other thing you need is a 3.3V USB-serial adapter ('FTDI') and some dupont wires to hook it up. The `/code` README has more information on how to flash the board, and how the firmware works. PIO makes it super easy to set up the development environment, it's like one click, don't be scared!!

## Usage

### User Interface

There's really just one button, on the back near the top of the board and not labelled because I'm dumb. When off, press once briefly to turn on. When on, a long-press will enter brightness setting mode, and a short press will rotate between brightnesses. A long-press again will go back to normal operation. Hold for 5s and release to turn off.

You can ignore the PROG button entirely unless you're planning to hack on the firmware, it does nothing other than put it into programming mode on power up.

If untouched for ~4h it will power itself off.

### Power

**Batteries**: Battery life should be approx 30 hours while operating. After the season I suggest removing the batteries as they will run down slowly over time since there is no hard power switch (a couple years). If you need more batteries, they are readily available type CR2032. Find them at the dollar store or on Amazon. The PKCELL ones I sent are cheap on Amazon and seem to not be trash.

**External power**: 🔴🔴 **NOTE: Don't supply external power with batteries installed!** 🔴🔴 In theory you could power this with any 4-16VDC supply (@ 10mA or so). It should run reliably at 5V though, so a cheerful USB charger would be perfect. There's an external power connector on the back of the board; if I thought you might want to hook this up, I included a pigtail with the appropriate connector already on it. The connector is a JST-PH (2.0mm pitch). +/- as marked on the board.

## Design
### Hardware

Goals / Met?:
* **Cheap** / Total cost incl. expedited shipping for 10 boards was \$153 (\$15.30 ea). I think so, though I would've liked to see $10.
* **Entirely manufactured/assembled by JLCPCB** / Mostly. LEDs were hand assembled, but I didn't expect them to be manufacturable anyway as they are mounted upside down.
* **Slim for mailing** / Yes
* **Battery life >= 24h with standby life of > 1 month** / Measured operating current with the included effects at full brightness is ~4-7mA. With 220mA CR2032s, that should be 30-50h. Standby current is <5µA, which is longer than the 3y shelf-life of the batteries.
* **Light/balanced enough with batteries to hang comfortably on a tree, or sit on a small stand** / Seems to work well for me.

#### Tools

I use [KiCad EDA](https://kicad.org/) exclusively for hardware design. [Inkscape](https://inkscape.org/) was used to create the simple artwork on the silkscreen and place the LED drill hits, with [Svg2Shenzhen](https://github.com/badgeek/svg2shenzhen) to export to Kicad. [FreeCAD](https://www.freecadweb.org/) was used for 3D modeling of the stand.

#### Design decisions

##### Power supply

It needed to be battery powered, as there is no standard power source in a Christmas tree. I considered hacking in to LED christmas light bulbs to tap power, but there doesn't seem to be a standard for these and it's not a convenient way to tap power anyway, since they seem to usually be wired in series, which would require a shunt supply, and are AC which complicates too. Same problems with old-fashioned incandescents.

Which really left two choices - coin cell(s) or AAAs. AAAs have much more capacity, but have two major issues for this design: the voltage dropped over the driver and LED would require at least 3 cells (3.5V bare minimum, when near empty) or a boost converter. 3 cells (even 2) wouldn't fit within the PCB envelope, and they would be relatively heavy and likely to make it hang funny or require a large stand. So it'd have to be a single-cell solution - possible with a fancy specialized boost converter, but expensive and relatively bulky (see 'cheap' and 'slim' design goals).

I considered taping a small lithium-ion cell to the back and including a USB charger on board, but these cells are not readily available in North America and the slow boat would be too slow in this case. JLCPCB also won't assemble USB connectors, and both these and the batteries are kinda expensive. I'm not *sure* the voltage would be sufficient, either.

So two coin cells (to maximize capacity and get to the necessary voltage) it is, in the largest standard format that I could cram on the board. The disposable-ness and temporary-usefullness of this choice doesn't make me happy, but I suppose it's a bit of a disposable and temporary item anyway 😕.

Maybe the 2021 version will do something more graceful here and attempt a life beyond the holiday season 😈.

##### Microcontroller

Basically I just picked the cheapest thing on the JLCPCB Basic Parts list that I was familiar with (STM32, AVR, or MSP430) and made sure it would do the job (really the only thing that mattered much was the standby power consumption, and that it could generate a 4MHz output clock for the MBI's PWM). It is an STM32F030F4 with 16KB flash and 4KB RAM and can run at up to 48MHz. Power consumption is linear with clock rate, so I will run it at 8MHz, which is as slow as possible to be able to generate a 4MHz clock for the MBI5043 to get complete PWM cycles at 60fps (65536 * 60 = 3.93MHz). I could run it faster since it sleeps when not actively drawing a frame, and it could be similar consumption or even save power, while offering increased processing power for fancy effects (what, though?), but it will probably just make it more power hungry. Thanks physics.

##### LED Driver

I wanted at least 10 constant-current outputs, at least 10-bit PWM, and a SPI-ish interface (I2C is slow, a pain to bit-bang, and power hungry). Considering JLCPCB's parts list, the only candidate really was Macroblock MBI5043. It has a weird interface for the latch that precludes use of the hardware SPI peripheral, and not a great datasheet, but it seems simple enough to bit-bang.

##### Power control

Toggle switches are relatively expensive and can't be placed by JLCPCB assembly service, so I preferred to avoid them. That means soft power, and to minimize parts, it means the microcontroller must have a standby mode with current consumption in low µA that can wake from a button press. The STM32F030 does so (~1.7µA). It also means everything else including the 3.3V regulator and LED driver must have a low current in standby too. Hence the selection of slightly more expensive HT7533 regulator, and using a P-channel power switch for the LED driver which has no standby mode and current consumption of several mA with all LEDs off (it may be less in practice without the PWM clock running). This also enables auto-power-off.

When powered down, standby consumption comes from HT7533 (2.5µA) + STM32 (1.7µA) + Q1 leakage (100nA) is ~4.5µA standby current. This should give ~5 years standby battery life. Good enough for this purpose. The traditional 'jellybean' 1117 regulator consumes 4mA, and without the power switch on the MBI5043 (guessing 2-4mA from the datasheet), it'd probably barely last a day.

### Software

#### Toolchain / Environment

* [PlatformIO](https://platformio.org/) build tools, to make hacking more accessible and build setup easy
* [libopencm3](https://github.com/libopencm3/libopencm3) 'convenience' library to interface with the STM32 peripherals (note: PIO will pull this for you automatically). I've used this before. It's nothing special, and for what little I'm using of it here, probably didn't save much time over just writing the registers. But time is time. Sorry, I don't know Arduino and its approach is too high level for me.
* [C++17](https://en.cppreference.com/w/cpp/17). I almost tried [Rust](https://www.rust-lang.org/what/embedded) for this, but opted for something I knew at least a little bit of (and that you might know a little bit of too) due to the crunch. Sorry, you can't run Node on this hardware.

#### Structure

See `code/README.md` for more information.

The structure is fairly typical of embedded programs. Execution is driven by a 1/60s timer interrupt, which wakes the processor from sleep. The ISR for this interrupt sends a frame to the LED driver if one is ready, and the mainloop then executes to draw the next frame and handle the button. After that works is complete, the processor goes back to sleep to wait for the next frame time.

I originally envisioned swapping function pointers to implement the effects, but in classic over-engineered fashion, this lead to templated callable structs that inherit from a base `MBIEffect` class and use dynamic dispatch. Why not.

There's not much else of interest there, but I welcome harsh code reviews.

## Mistakes / Unexpectedness

* I sprung for fancy Immersion Gold finish PCBs because they'd look nicer, even though there's barely any exposed copper on the front. I should have realized this could be used for the artwork and gold-on-black would be much blingier than white-on-black, or even 3-colour art!
* I did not think to match the LED reference designations with their output # / buffer index. At least it's only an off-by-one since the references are 1-indexed rather than random.
* 3 weeks is not enough time to design boards, have them made and delivered, write software, and then deliver to recipients. Not enough time *at all*.
* Read the datasheets *carefully*. The pin I wired up as the PWM clock does indeed operate as a timer output, but only on some device variants, and not the one I have. Could've just used one of the free pins instead...this is why there is a bodge wire on your board.
* These 'lighting' LEDs are...melty... They tolerate almost no heat from the soldering iron. Not used to components being made with thermoplastics.