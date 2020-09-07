# cabin-fever

**TweenPin** is a 3/4 scale custom pinball machine. It consists of all the traditional pinball machine materials: a wood cabinet and playfield, lamps, solenoids, high current power supplies, flippers, targets, pop bumpers, 1 1/8" steel ball, tempered glass, a scoring display and of course an Atmel AVR (ATmega16) powered master control board.

Since the advent of the first commercially successful pinball machine (Gottlieb's Baffle Ball in 1931) pinball has seen its ups and downs. Commercial machines have quite a rich history, with varied and unique features. In more recent years, they've achieved a highly collectible status for home use.

Creating one's own pinball machine, utilizing real machine components, is a challenging and appealing project to many people. It offers the opportunity to build a fully custom, complex, and lasting entertainment device for the enjoyment of the entire family. There are however, many difficulties with such a project. Of concern is the overall style of the machine (and art), sourcing the wide range of necessary components, and of course there is the machine control issue. The project truly is a multidisciplinary effort in electronic design, embedded software, woodworking, mechanical design and art.

**TweenPin utilizes three major electronic subsystems**

**Custom unregulated power supplies** step down wall power voltages down to 6 and 24 AC volts RMS using two high current transformers. Three diode bridges and two capacitors result in the generation of about 6, 8 and 36 DC volts. The 6VDC is used to power the machine's lamps. The 8VDC is used to power the circuit boards, and the 36VDC is used to power the solenoids.

**A custom display board** shows the current score and other status. It consists of four dual 1" tall yellow common cathode seven segment displays and a MAX7219. An LM317 performs onboard 5.0v linear regulation of the 8v input. The MAX part is a serial interfaced, programmable, 8 digit multiplexing, direct segment driver. The control board occasionally sends updates via SPI commands when a change to the display is required.

**A custom control board** is both the brains and brawn of the electronic subsystems. It consists mainly of a single ATmega16 master controller, IO expanders, filtered inputs, MOSFET high current outputs and euro style pluggable connectors. An LM317 performs onboard 5.0v linear regulation of the 8v input. The ATmega16 holds all the logic for rules and controls on the game.
