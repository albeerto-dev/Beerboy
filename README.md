# Beerboy - *Automation for homebrewing*

Beerboy is an `Arduino` based brewing controller for homebrewers.
![logo image](https://github.com/albeerto-dev/Beerboy/blob/master/Logo_BEERBOY.png =200x200)

## Functionalities

It's for a single Input / single Output system.  It's perfect for 
BIAB systems. "Beerboy 2.0" can be used also for classic ALL GRAIN just by 
swapping the plug of the heating element of the mash kettle 
with the one of the boil kettle (you'll also need to move the
temp probe in the boil kettle). If your AG system has a sparge kettle/pot 
this controller can't manage it.

It can be used in two `modes`:
* __AUTO__
* __MANUAL__
#### AUTO
Everything is set before the start of the brewday. The mash temperature is controlled by 
a `PID algorithm`.
In sequence you can set up : 
1. Number of mash steps 
2. Temperature and time in minutes of each step of the mash 
3. Boil duration in minutes 
4. Number of hop jetty 
5. Time of each hop jetty 
6. Hopstand duration in minutes.

#### MANUAL
It's like a classic temperature `PID controller`. You can olny set the 
temperature desired and change it whenever you want.
### Hardware components
* Arduino Uno R3 (or equivalent) x 1
* Buzzer x 1
* Button x 3
* Display LCD 16x2 x 1
* Temperature probe DS18B20 x 1
* Wires
* Resistor 10.000 Ohm x 3 [buttons]
* Resistor 2.000 Ohm x 1 [LCD frontlight]
* Resistor 220 Ohm x 1 [LCD backlight]
* Resistor 100 Ohm x 1 [buzzer]
* Resistor 4.700 Ohm x 1 [DS18B20]
* SSR 25A / 40A x 1

## Video demonstration
work in progress
## Wiring instructions
That's the precise scheme. AC Input should be your main plug. AC Output should be your heating element.
![Image of wiring](https://github.com/albeerto-dev/Beerboy/blob/master/Beerboy%20scheme.png)
## Version
V 1.0
## About
Beerboy 2.0 is a brewing controller developed by `Alberto M. Ramagini`.
